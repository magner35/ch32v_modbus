/*
 * Created on Wed Sep 16 2026
 *
 * Copyright (c) 2026 by magner mr.jaedong@gmail.com
 */

#include "eeprom.h"

/* Журнал в последней странице flash. ВАЖНО: страница CH32V003 = 1 КБ
 * (standard erase), а не 256 Б. Запись 16 Б, 64 слота. Аварийное
 * сохранение (по PVD) — ТОЛЬКО программирование четырёх слов (~0.3 мс),
 * без стирания. Когда журнал подходит к концу, при следующем старте
 * страница стирается и последняя запись переписывается в начало
 * (стирание — только при стабильном питании).
 * Прошивка не должна заходить за 0x08003C00 (см. контроль размера). */
#define CRIT_PAGE 0x08003C00UL

/* Слот = 64-байтовый блок flash: запись выполняется ОДНОЙ быстрой
 * операцией (BufLoad×16 + ProgramPage_Fast, ~0.1 мс) — при крутом спаде
 * питания пословная запись через FLASH_ProgramWord не успевала. */
#define CRIT_SLOTS (1024 / 64)
#define CRIT_MAINTAIN_SLOT (CRIT_SLOTS / 2) /* порог обслуживания при старте */

#define CRITICAL_MAGIC   0x43524954UL /* "CRIT" */
#define CRITICAL_VERSION 5            /* v5: слот 64 Б, быстрая запись */

typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t seq; /* номер сохранения: больше — новее */
    uint32_t crc; /* crc32 поля payload */
    uint32_t pad; /* выравнивание payload до 8 */
    critical_payload_t payload;
} critical_record_t;

_Static_assert(sizeof(critical_record_t) == 64, "record layout changed");

/* Диагностика (читается по Modbus, см. meter.c):
 * bit0 — при старте найдена валидная запись
 * bit1 — последняя critical_save записала flash
 * bit2 — контрольное чтение записи подтвердило данные */
volatile uint8_t critical_debug = 0;

/* Состояние журнала (восстанавливается при старте) */
static uint8_t next_slot = 0;
static uint16_t last_seq = 0;
static critical_record_t last_rec = {0};

static uint32_t crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    while (len--)
    {
        crc ^= *data++;
        for (uint8_t bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
    }
    return ~crc;
}

static bool record_valid(const critical_record_t *r)
{
    return (r->magic == CRITICAL_MAGIC) &&
           (r->version == CRITICAL_VERSION) &&
           (r->crc == crc32((const uint8_t *)&r->payload, sizeof(r->payload)));
}

static const critical_record_t *slot_ptr(uint8_t slot)
{
    return (const critical_record_t *)(CRIT_PAGE + (uint32_t)slot * sizeof(critical_record_t));
}

/* Сколько ведущих валидных записей в странице; *clean = true, если
 * остановка на стёртом слоте (журнал цел) или страница заполнена до конца.
 * false = наткнулись на мусор: чужая версия формата или оборванная запись */
static uint8_t scan_page(bool *clean)
{
    uint8_t n = 0;
    *clean = true;
    for (uint8_t s = 0; s < CRIT_SLOTS; s++)
    {
        const critical_record_t *r = slot_ptr(s);
        if (r->magic == 0xFFFFFFFFUL)
            break; /* дальше стёрто — чисто */
        if (!record_valid(r))
        {
            *clean = false; /* мусор: нельзя программировать поверх без стирания */
            break;
        }
        last_rec = *r;
        n++;
    }
    return n;
}

static void flash_clear_errors(void)
{
    /* запись при просевшем питании оставляет флаги ошибок и блокирует
     * контроллер flash до сброса — снимаем перед каждой операцией */
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_WRPRTERR);
}

static void flash_erase_page(void)
{
    /* быстрое стирание работает блоками по 64 Б — проходим все блоки
     * страницы (совместимо и с 1К-семантикой: повторное стирание безвредно) */
    FLASH_Unlock();
    FLASH_Unlock_Fast();
    flash_clear_errors();
    for (uint8_t blk = 0; blk < 1024 / 64; blk++)
        FLASH_ErasePage_Fast(CRIT_PAGE + 64UL * blk);
    FLASH_Lock_Fast();
    FLASH_Lock();
}

static void flash_program_record(uint32_t addr, const critical_record_t *rec)
{
    /* быстрая запись: 64-байтовый блок одной операцией через буфер
     * контроллера — в разы быстрее пословной, переживает крутой спад VDD */
    const uint32_t *w = (const uint32_t *)rec;
    FLASH_Unlock_Fast();
    flash_clear_errors();
    FLASH_BufReset();
    for (uint8_t i = 0; i < sizeof(*rec) / 4; i++)
        FLASH_BufLoad(addr + 4UL * i, w[i]);
    FLASH_ProgramPage_Fast(addr);
    FLASH_Lock_Fast();
}

bool critical_init(critical_payload_t *data)
{
    bool clean;
    uint8_t n = scan_page(&clean);

    if (n > 0)
    {
        next_slot = n;
        last_seq = last_rec.seq;
        critical_debug |= 0x01;
        if (last_rec.seq & CRITICAL_SEQ_PVD_MARK)
            critical_debug |= 0x08; /* прошлая сессия завершилась PVD-сохранением */
        if (data)
            *data = last_rec.payload;
    }
    else
    {
        next_slot = 0;
        last_seq = 0;
    }

    /* Обслуживание при стабильном питании:
     * - журнал почти заполнен — переносим последнюю запись в начало;
     * - в странице мусор (запись другой версии формата или оборванная) —
     *   программировать поверх нельзя (flash пишет только 1→0), стираем. */
    if (n >= CRIT_MAINTAIN_SLOT || !clean)
    {
        __disable_irq();
        flash_erase_page();
        if (n > 0)
            flash_program_record(CRIT_PAGE, &last_rec);
        __enable_irq();
        next_slot = (n > 0) && record_valid((const critical_record_t *)CRIT_PAGE) ? 1 : 0;
    }

    return (n > 0);
}

uint16_t critical_seq(void)
{
    return last_seq & CRITICAL_SEQ_PVD_MARK_INV;
}

bool critical_save(const critical_payload_t *data, bool pvd_mark)
{
    uint8_t slot = next_slot;

    if (slot >= CRIT_SLOTS)
    {
        /* журнал переполнен (не должно случаться после обслуживания) —
         * аварийная очистка и запись в начало */
        __disable_irq();
        flash_erase_page();
        __enable_irq();
        slot = 0;
    }

    critical_record_t rec = {0}; /* нули: pad и payload.reserve детерминированы */
    rec.magic = CRITICAL_MAGIC;
    rec.version = CRITICAL_VERSION;
    rec.seq = ((uint16_t)(last_seq & CRITICAL_SEQ_PVD_MARK_INV) + 1);
    if (pvd_mark)
        rec.seq |= CRITICAL_SEQ_PVD_MARK; /* маркер: запись сделана по PVD */
    rec.payload = *data;
    /* reserve у вызывающего может быть мусором — CRC по фактическим байтам */
    rec.crc = crc32((const uint8_t *)&rec.payload, sizeof(rec.payload));

    uint32_t addr = CRIT_PAGE + (uint32_t)slot * sizeof(rec);
    __disable_irq();
    flash_program_record(addr, &rec);
    __enable_irq();

    if (!record_valid((const critical_record_t *)addr))
    {
        /* слот испорчен (программирование поверх старых битов или обрыв
         * при просевшем питании): самолечение — стереть страницу и
         * записать в начало, иначе журнал застрянет на этом слоте */
        __disable_irq();
        flash_erase_page();
        flash_program_record(CRIT_PAGE, &rec);
        __enable_irq();
        slot = 0;
        addr = CRIT_PAGE;
        if (!record_valid((const critical_record_t *)addr))
            return false; /* и после стирания не легло — питание уже село */
    }

    /* запись проверена: обычным путём или после самолечения */
    critical_debug |= (0x02 | 0x04);
    next_slot = slot + 1;
    last_seq = rec.seq;
    last_rec = rec;
    return true;
}
