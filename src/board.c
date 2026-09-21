#include "board.h"

void BoardGpioInit(void)

{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    // RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

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

void BoardTimer2Init(void)
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

/* ── Конфигурация ── */
#define TIM_CLK_HZ 48000000UL
#define TIM_PRESCALER 48 // → 1 МГц (1 тик = 1 мкс)
#define TIM_TICK_HZ (TIM_CLK_HZ / TIM_PRESCALER)

/* ── Глобальные переменные (ISR ↔ main) ── */
volatile uint16_t cap_rising = 0;
volatile uint16_t cap_falling = 0;
volatile uint16_t prev_rising = 0;
volatile uint16_t period_ticks = 0;
volatile uint16_t pulse_ticks = 0;
volatile uint16_t overflow_cnt = 0;
volatile uint8_t meas_ready = 0;

void TIM2_IC_Init(void)
{
    /* Тактирование */
    RCC->APB2PCENR |= RCC_IOPAEN; // GPIOA
    RCC->APB1PCENR |= RCC_TIM2EN; // TIM2

    /* PA0 — floating input (TIM2_CH1) */
    GPIOA->CFGLR &= ~(0xF << (0 * 4));
    GPIOA->CFGLR |= (0x4 << (0 * 4)); // 0100 = float in

    /* Предделитель и автоперезагрузка */
    TIM2->PSC = TIM_PRESCALER - 1;
    TIM2->ATRLR = 0xFFFF;
    TIM2->SWEVGR = TIM_UG; // применить PSC

    /* ── CH1: Input Capture, TI1, rising edge ── */
    // CC1S = 01 → IC1 маппирован на TI1
    TIM2->CHCTLR1 &= ~(0x3 << 0);
    TIM2->CHCTLR1 |= (0x1 << 0); // CC1S = 01
    // IC1F = 0100 (фильтр N=4, fS = fDTS) — подавление дребезга
    TIM2->CHCTLR1 &= ~(0xF << 4);
    TIM2->CHCTLR1 |= (0x4 << 4);

    /* ── CH2: Input Capture, тоже TI1, falling edge ── */
    // CC2S = 10 → IC2 маппирован на TI1 (cross-capture!)
    TIM2->CHCTLR1 &= ~(0x3 << 8);
    TIM2->CHCTLR1 |= (0x2 << 8); // CC2S = 10
    // IC2F = 0100
    TIM2->CHCTLR1 &= ~(0xF << 12);
    TIM2->CHCTLR1 |= (0x4 << 12);

    /* Полярность: CH1 = rising, CH2 = falling */
    TIM2->CCER &= ~(TIM_CC1P | TIM_CC2P);
    TIM2->CCER |= TIM_CC2P;            // CH2 — по спаду
    TIM2->CCER |= TIM_CC1E | TIM_CC2E; // включить оба

    /* Прерывания: CC1, CC2, Update (переполнение) */
    TIM2->DMAINTENR |= TIM_CC1IE | TIM_CC2IE | TIM_UIE;
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM2->CTLR1 |= TIM_CEN; // старт
}

void TIM2_IRQHandler(void) __attribute__((interrupt));
void TIM2_IRQHandler(void)
{
    uint16_t sr = TIM2->INTFR;

    /* Переполнение таймера */
    if (sr & TIM_UIF)
    {
        TIM2->INTFR = ~TIM_UIF;
        overflow_cnt++;
    }

    /* Rising edge (CH1) — начало нового периода */
    if (sr & TIM_CC1IF)
    {
        cap_rising = TIM2->CH1CVR;
        TIM2->INTFR = ~TIM_CC1IF;

        // Период = текущий rising − предыдущий rising (с учётом переполнений)
        if (overflow_cnt == 0)
        {
            period_ticks = cap_rising - prev_rising;
        }
        else if (overflow_cnt == 1)
        {
            period_ticks = (0xFFFF - prev_rising) + cap_rising + 1;
        }
        else
        {
            // Слишком длинный период — сигнал пропал
            period_ticks = 0;
        }
        overflow_cnt = 0;
        prev_rising = cap_rising;
    }

    /* Falling edge (CH2) — конец импульса */
    if (sr & TIM_CC2IF)
    {
        cap_falling = TIM2->CH2CVR;
        TIM2->INTFR = ~TIM_CC2IF;

        // Длительность импульса = falling − последний rising
        pulse_ticks = cap_falling - prev_rising;
        meas_ready = 1; // данные готовы для обработки в main loop
    }
}