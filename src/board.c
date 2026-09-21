#include "board.h"

void BoardGpioInit(void)

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

void BoardTimer2Init_(void)
{
    RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

    TIM2->ATRLR = 0xffff;
    TIM2->PSC = 0xffff;
    TIM2->SWEVGR = TIM_UG;

    TIM2->CTLR1 |= TIM_CEN;
}

void BoardTimer2Init__(void)
{
    /* 1. Включаем тактирование TIM2 (шина APB1) */
    RCC->APB1PCENR |= RCC_TIM2EN;
    /* 2. Сбрасываем таймер */
    TIM2->CTLR1 = 0;
    /* 3. Настраиваем предделитель и автозагрузку
     *    Пример: F_tim = 24 MHz / (PSC+1) / (ATRLR+1)
     *    При PSC=24000-1, ATRLR=1000-1 → 1 Гц (1 сек) */
    TIM2->PSC = 48000 - 1;   // 24 MHz / 24000 = 1000 Hz
    TIM2->ATRLR = 10000 - 1; // 1000 Hz / 1000 = 1 Hz
    /* 4. Применяем предделитель (генерация update-события) */
    TIM2->SWEVGR = TIM_UG;
    /* 5. Запускаем таймер */
    TIM2->CTLR1 |= TIM_CEN;
}

void BoardTimer1Init(void)
{
    RCC->APB2PCENR |= RCC_APB2Periph_TIM1;

    TIM1->ATRLR = 0xffff;
    TIM1->PSC = 0xffff;
    TIM1->SWEVGR = TIM_UG;

    TIM1->CTLR1 |= TIM_CEN;
}

// Timer Initialize 1ms
void Timer_Initialise(void)
{
    RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
    TIM1->CTLR1 |= TIM_ARPE;
    TIM1->CTLR2 = TIM_MMS_1;
    // count up per 1sec
    // 48000 * 1000 = 48000000
    TIM1->ATRLR = 480;
    TIM1->PSC = 100 - 1;
    TIM1->RPTCR = 0;
    NVIC_EnableIRQ(TIM1_UP_IRQn);
    TIM1->INTFR = ~TIM_FLAG_Update;   // 0x0001 // 10.4.5 Interrupt Status Register (TIM1_INTFR)
    TIM1->SWEVGR = TIM_UG;            // 0x0001 // 10.4.6 Event Generation Register (TIM1_SWEVGR)
    TIM1->DMAINTENR |= TIM_IT_Update; // 0x0001 // 10.4.4 DMA/Interrupt Enable Register (TIM1_DMAINTENR)
    // TIM1 Enable
    TIM1->CTLR1 |= TIM_CEN;
}
