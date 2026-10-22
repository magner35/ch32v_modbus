#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdbool.h>
#include <stdint.h>

#define LPOINTS 10

/* ── Обёртки «значение + диапазон» ─────────────────────────────────────── */
typedef struct
{
    float value; /* значение */
    float range; /* верхняя граница; 0 = без ограничений (float) */
} fset_t;

typedef struct
{
    uint8_t value;
    uint8_t range; /* верхняя граница (нижняя — в дескрипторе) */
} uset_t;

typedef struct
{
    uint8_t value;
    uint8_t range; /* количество вариантов, мин всегда 0 */
} eset_t;

typedef struct
{
    bool value; /* у булева диапазона нет */
} bset_t;

/* ── Типы настроек ─────────────────────────────────────────────────────── */
typedef enum
{
    ST_BOOL,
    ST_ENUM,
    ST_U8,
    ST_I8,
    ST_U16,
    ST_U32,
    ST_FLOAT
} setting_type_t;

/* ── Значения enum-настроек (числа = значения в регистрах/flash) ── */
enum
{
    OFF,
    ON
};

enum
{
    UNITS_ML,
    UNITS_L,
    UNITS_M3
}; /* units_total, units_gtotal, multiplier_units */
enum
{
    TBASE_SEC,
    TBASE_MIN,
    TBASE_HOUR
}; /* units_time_base */
enum
{
    PARITY_NONE,
    PARITY_EVEN,
    PARITY_ODD
}; /* interfaceRS485_parity */
enum
{
    AOUT_RATE,
    AOUT_TOTAL,
    AOUT_GTOTAL
}; /* aout_parameter */

enum
{
    BAUD_1200,
    BAUD_2400,
    BAUD_4800,
    BAUD_9600,
    BAUD_14400,
    BAUD_19200,
    BAUD_28800,
    BAUD_38400,
    BAUD_57600,
    BAUD_115200
};

enum
{
    ILEVEL_40,
    ILEVEL_20,
    ILEVEL_10,
    ILEVEL_5
};

enum
{
    FILTERQ_OFF,
    FILTERQ_12,
    FILTERQ_25,
    FILTERQ_37,
    FILTERQ_50
};

enum
{
    FILTERHF_OFF,
    FILTERHF_300,
    FILTERHF_600,
    FILTERHF_1200,
    FILTERHF_2400
};

enum
{
    IMODE_RS485,
    IMODE_PULSE,
};

/* ── Единственное место правды: все настройки прибора ──────────────────── */
/* Порядок строк = порядок полей в settings_t / set_t и порядок SET_*-индексов.
 * Правила колонок:
 *   BOOL  — ограничений нет:         X(имя, BOOL,  -, -,      def)
 *   ENUM  — только число вариантов:  X(имя, ENUM,  -, кол-во, dv)
 *   U8/I8/U16/U32 — min/max того же типа, что и параметр
 *   FLOAT — без ограничений:         X(имя, FLOAT, -, -,      def)
 *        имя                        тип    мин     макс      дефолт        */
#define SETTINGS_TABLE(X)                                                                             \
    X(multiplier_parameter, BOOL, -, -, 0)               /* 0 - K-фактор, 1 - цена импульса */        \
    X(multiplier_units, ENUM, -, 3, UNITS_ML)            /* 0 - миллилитр, 1 - литр, 2 - метр куб. */ \
    X(multiplier_value_average, FLOAT, -, -, 1.0f)       /* значение множителя */                     \
    X(linearisation, BOOL, -, -, 1)                      /* 0 - выкл, 1 - сплайн-Б */                 \
    X(meter_correction, FLOAT, -, -, 1.0f)               /* поправочный коэффициент */                \
    X(serial_number, U32, 0, 999999, 123456)             /* серийный номер прибора */                 \
    X(units_time_base, ENUM, -, 3, TBASE_MIN)            /* 0 - сек, 1 - мин, 2 - час (TODO) */       \
    X(units_rate, ENUM, -, 3, UNITS_L)                   /* 0 - миллилитр, 1 - литр, 2 - метр куб. */ \
    X(units_total, ENUM, -, 3, UNITS_L)                  /* 0 - миллилитр, 1 - литр, 2 - метр куб. */ \
    X(aout_parameter, ENUM, -, 3, AOUT_RATE)             /* переменная для аналогового выхода */      \
    X(aout_treshold_4ma, FLOAT, -, -, 0.0f)              /* нижний порог */                           \
    X(aout_treshold_20ma, FLOAT, -, -, 0.0f)             /* верхний порог */                          \
    X(rate_damping, U8, 0, 99, 0)                        /* сек демпфирования*/                       \
    X(rate_averaging, U8, 0, 255, 0)                     /* количество импульсов для усреднения */    \
    X(input_sens, ENUM, -, 4, ILEVEL_40)                 /* чувствительность по входу */              \
    X(sensor_filter_freq_high, ENUM, -, 5, FILTERHF_OFF) /* индекс, не Гц: сейчас 4500 Гц (TODO) */   \
    X(sensor_filter_freq_low, U8, 0, 255, 0)             /* ФНЧ Гц */                                 \
    X(sensor_filter_duty, ENUM, -, 5, FILTERQ_OFF)       /* Q коэф.заполнения % */                    \
    X(interfaceRS485_slave_address, U8, 1, 247, 1)       /* адрес устройства modbus */                \
    X(interfaceRS485_baudrate, ENUM, -, 10, BAUD_9600)   /* rs-485 baudrate */                        \
    X(interfaceRS485_parity, ENUM, -, 3, PARITY_NONE)    /* 0 - none, 1 - even, 2 - odd */            \
    X(interfaceRS485_stopbits, BOOL, -, -, 0)            /* 0 - 1 стоп-бит, 1 - 2 */                  \
    X(interfaceRS485_delay, U8, 0, 255, 5)               /* задержка ответа, мс */                    \
    X(interfaceRS485_mode, ENUM, -, 2, IMODE_RS485)      /* альтернативная фунция (TODO) */           \
    X(aout_calibration_low, FLOAT, -, -, 0.0f)           /* нижнее клибровочное значение АВ */        \
    X(aout_calibration_high, FLOAT, -, -, 0.0f)          /* верхнее калибровочное значение АВ */

/* ── Отображения «символьный тип → C-тип / обёртка / инициализатор» ────── */

#define ST_CTYPE_BOOL bool
#define ST_CTYPE_ENUM uint8_t
#define ST_CTYPE_U8 uint8_t
#define ST_CTYPE_I8 int8_t
#define ST_CTYPE_U16 uint16_t
#define ST_CTYPE_U32 uint32_t
#define ST_CTYPE_FLOAT float

#define ST_WRAPPER_BOOL bset_t
#define ST_WRAPPER_ENUM eset_t
#define ST_WRAPPER_U8 uset_t
#define ST_WRAPPER_I8 uset_t
#define ST_WRAPPER_U16 uset_t
#define ST_WRAPPER_U32 fset_t /* serial_number до 999999 точно представим во float */
#define ST_WRAPPER_FLOAT fset_t

/* Инициализаторы factory: value ← «дефолт», range ← «кол-во»/«макс».
 * У BOOL поля range нет; у FLOAT range не заполняется (0 = без ограничений) */
#define ST_INIT_BOOL(n, mn, mx, dv) .n = {.value = (dv)},
#define ST_INIT_NUM(n, mn, mx, dv) .n = {.value = (dv), .range = (mx)},
#define ST_INIT_FLOAT(n, mn, mx, dv) .n = {.value = (dv)},
#define ST_INIT_ENUM ST_INIT_NUM
#define ST_INIT_U8 ST_INIT_NUM
#define ST_INIT_I8 ST_INIT_NUM
#define ST_INIT_U16 ST_INIT_NUM
#define ST_INIT_U32 ST_INIT_NUM

/*----------------------------------------------------------------------------*/
/*
 * Коэффициенты пересчёта вычисляются на месте из таблиц (раньше кэшировались
 * в csettings: 9 float + копии units - теперь их поля удалены ради ОЗУ).
 * Таблицы - во флеше, выражение из 2-4 float-операций дешевле кэша в SRAM.
 * Входы (mmeter/pmsettings) атомарны поштучно (ATOM в storePropertyValue).
 */
extern const float pow1000_tab[3]; /* units_* 0..2 = мл/л/м³ */
extern const float pow60_tab[3];   /* time_base 0..2 = с/мин/ч */

#define LPM_MULTIPLIER 60

#define COEF_KF() (pow1000_tab[g_settings.multiplier_units] * g_settings.meter_correction)
#define COEF_RATE() (pow1000_tab[g_settings.multiplier_units] / pow1000_tab[g_settings.units_rate] * pow60_tab[g_settings.units_time_base] * g_settings.meter_correction)
#define COEF_RATELPM() (pow1000_tab[g_settings.multiplier_units] * LPM_MULTIPLIER * g_settings.meter_correction)
#define COEF_TOTAL() (pow1000_tab[g_settings.units_total])

/* ── Индексы настроек: SET_<имя> ───────────────────────────────────────── */
#define SETTINGS_INDEX(n, t, mn, mx, dv) SET_##n,
enum
{
    SETTINGS_TABLE(SETTINGS_INDEX)
    SETTINGS_COUNT
};

/* ── Рабочая копия значений (её образ уходит во flash) ─────────────────── */
#define SETTINGS_FIELD(n, t, mn, mx, dv) ST_CTYPE_##t n;
typedef struct
{
    SETTINGS_TABLE(SETTINGS_FIELD)
} settings_t;

/* ── Значения + диапазоны для меню/HMI ─────────────────────────────────── */
#define SET_FIELD(n, t, mn, mx, dv) ST_WRAPPER_##t n;
typedef struct
{
    SETTINGS_TABLE(SET_FIELD)
} set_t;

/* ── Лимиты: min/max/def тем же типом, что и параметр ──────────────────── */
/* ENUM хранит только число вариантов (мин всегда 0).
 * BOOL и FLOAT ограничений не имеют — только значение по умолчанию.       */
typedef union
{
    struct
    {
        uint8_t min, max, def;
    } u8; /* ST_U8   */
    struct
    {
        int8_t min, max, def;
    } i8; /* ST_I8   */
    struct
    {
        uint16_t min, max, def;
    } u16; /* ST_U16 */
    struct
    {
        uint32_t min, max, def;
    } u32; /* ST_U32 */
    struct
    {
        uint8_t count, def;
    } e; /* ST_ENUM */
    struct
    {
        uint8_t def;
    } b; /* ST_BOOL */
    struct
    {
        float def;
    } f; /* ST_FLOAT */
} setting_limits_t;

/* ── Дескриптор настройки (const → хранится во flash) ──────────────────── */

typedef struct
{
    uint8_t type;  /* setting_type_t */
    uint8_t words; /* регистров Modbus: 1 или 2 */
    setting_limits_t lim;
} setting_desc_t;

/* Построение строки дескриптора по типу (words: 1 или 2 регистра Modbus) */
#define ST_DESC_BOOL(n, mn, mx, dv) {ST_BOOL, 1, {.b = {.def = (dv)}}},
#define ST_DESC_ENUM(n, mn, mx, dv) {ST_ENUM, 1, {.e = {.count = (mx), .def = (dv)}}},
#define ST_DESC_U8(n, mn, mx, dv) {ST_U8, 1, {.u8 = {.min = (mn), .max = (mx), .def = (dv)}}},
#define ST_DESC_I8(n, mn, mx, dv) {ST_I8, 1, {.i8 = {.min = (mn), .max = (mx), .def = (dv)}}},
#define ST_DESC_U16(n, mn, mx, dv) {ST_U16, 1, {.u16 = {.min = (mn), .max = (mx), .def = (dv)}}},
#define ST_DESC_U32(n, mn, mx, dv) {ST_U32, 2, {.u32 = {.min = (mn), .max = (mx), .def = (dv)}}},
#define ST_DESC_FLOAT(n, mn, mx, dv) {ST_FLOAT, 2, {.f = {.def = (dv)}}},

/* ── Образ страницы настроек во flash ──────────────────────────────────── */
#define SETTINGS_MAGIC 0x4D534554UL /* "MSET" */
#define SETTINGS_VERSION 1

typedef struct
{
    uint32_t magic;
    uint16_t version; /* версию поднимать при любом изменении settings_t */
    uint16_t seq;     /* номер сохранения: какая из страниц новее */
    uint32_t crc32;   /* по payload */
    settings_t payload;
} settings_flash_t;

/* ── Данные и API ──────────────────────────────────────────────────────── */
extern settings_t g_settings;     /* рабочая копия */
extern const set_t factory_meter; /* заводские значения + диапазоны */
extern const setting_desc_t g_settings_desc[SETTINGS_COUNT];
extern const uint16_t g_settings_offs[SETTINGS_COUNT]; /* offsetof(settings_t, поле) */

extern float lpoint_forward_frequency[LPOINTS]; /* таблица линеаризации */
extern float lpoint_forward_value[LPOINTS];
extern const float factory_lpoint_forward_frequency[LPOINTS]; /* заводские */
extern const float factory_lpoint_forward_value[LPOINTS];
extern uint8_t lpoints_count; /* эффективное число точек после нормализации */

uint8_t settingsLpoints(void); /* нормализация таблицы + подсчёт точек (0 = мало точек) */

void meterInit(void);                        /* входная точка: flash → (иначе) заводские */
void settings_load_factory(void);            /* g_settings ← заводские из дескрипторов */
bool settings_check(uint8_t idx, float val); /* проверка значения по типу и лимитам */
float settings_get(uint8_t idx);             /* чтение значения по индексу (как float) */
bool settings_set(uint8_t idx, float val);   /* запись по индексу с проверкой лимитов */
bool settings_save_flash(void);              /* TODO: ping-pong двух страниц */
bool settings_load_flash(void);              /* TODO: выбор валидного образа по seq/crc */

#endif // _SETTINGS_H_
