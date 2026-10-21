#include "board.h"

/* Прежняя реализация на SPL — для справки/отката, в прошивку не линкуется. */
void BoardGpioInit_SPL(void)

{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    // LED
    GPIO_InitStructure.GPIO_Pin = LED1_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED1_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = LED2_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED2_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GEN_OUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GEN_OUT_PORT, &GPIO_InitStructure);
}

/* Инициализация GPIO — прямые записи в регистры. Эквивалент
 * BoardGpioInit_SPL. Все пины — выходы push-pull 50 МГц:
 * CNF=00, MODE=11 → значение поля CFGLR = 0x3.
 * PD0, PD2 (LED2), PD3 (LED1), PC4 (GEN_OUT). */
void BoardGpioInit(void)
{
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;

    GPIOD->CFGLR = (GPIOD->CFGLR & ~((uint32_t)0xFu << 0 | (uint32_t)0xFu << 8 | (uint32_t)0xFu << 12)) | (0x3u << 0) | (0x3u << 8) | (0x3u << 12);
    GPIOC->CFGLR = (GPIOC->CFGLR & ~((uint32_t)0xFu << 16)) | (0x3u << 16);
}
