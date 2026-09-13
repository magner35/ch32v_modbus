#include "ch32v00x.h"
#include "eeprom.h"

#define EEPROM_ADDR 0x08003FC0

void save_Counter_To_Flash(uint32_t counter_value)
{
    __disable_irq();
    FLASH_Unlock_Fast();

    // GPIO_SetBits(GPIOD, GPIO_Pin_3);

    FLASH_ErasePage_Fast(EEPROM_ADDR);
    FLASH_ProgramWord(EEPROM_ADDR, counter_value);
    // FLASH_ProgramPage_Fast(EEPROM_ADDR);

    // GPIO_ResetBits(GPIOD, GPIO_Pin_3);

    FLASH_Lock_Fast();
    __enable_irq();
}
