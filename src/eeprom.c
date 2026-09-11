#include "ch32v00x.h"
#include "eeprom.h"

// Адрес для хранения данных (последняя страница 16KB Flash: 0x08000000 + 16384 - 64)
#define EEPROM_ADDR 0x08003FC0

// Функция сохранения счетчика (вызывается в обработчике PVD)
void save_Counter_To_Flash(uint32_t counter_value)
{
    // 1. Запрещаем все прерывания, чтобы избежать HardFault во время записи

    __disable_irq();
    // 2. Разблокируем Flash

    FLASH_Unlock();

    // 3. Очищаем флаги ошибок (на всякий случай)
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_WRPRTERR);
    
    GPIO_SetBits(GPIOD, GPIO_Pin_3);
    // 4. Стираем страницу (64 байта). Это занимает ~2.5 мс
    FLASH_ErasePage(EEPROM_ADDR);
    GPIO_ResetBits(GPIOD, GPIO_Pin_3);
    
    // 5. Записываем данные (минимальная единица записи - 16 бит / полуслово)
    // Записываем младшие 16 бит
    FLASH_ProgramHalfWord(EEPROM_ADDR, (uint16_t)(counter_value & 0xFFFF));
    // Записываем старшие 16 бит
    FLASH_ProgramHalfWord(EEPROM_ADDR + 2, (uint16_t)(counter_value >> 16));

    // 6. Блокируем Flash обратно
    FLASH_Lock();

    // 7. Разрешаем прерывания (хотя питание уже на исходе, это хорошая практика)
    __enable_irq();
}