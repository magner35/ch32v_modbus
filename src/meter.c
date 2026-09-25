/*
 * Created on Wed Sep 16 2026
 *
 * Copyright (c) 2026 by magner mr.jaedong@gmail.com
 */

#include "meter.h"
#include <stdint.h>
#include "debug.h"
#include "board.h"
#include "interpolation.h"
#include "Modbus.h"
#include "modbus_registers.h"

/* ── Тайминги ── */
#define TIM_CLK_HZ 48000000UL
#define TIM_PRESCALER 1 /* PSC=0 → 48 МГц */
#define TIM_TICK_HZ (TIM_CLK_HZ / TIM_PRESCALER)

/* ── Пороги фильтрации (целочисленные, без float!) ── */
#define FREQ_MIN_HZ 1UL
#define FREQ_MAX_HZ 4500UL
/* duty ∈ [0.10 ; 0.90]  ⇔  pulse*10 ∈ [period ; period*9] */
#define DUTY_NUM_MIN 1UL
#define DUTY_NUM_MAX 9UL

/* Периоды в тиках, соответствующие порогам частоты */
#define PERIOD_MAX_TICKS (TIM_TICK_HZ / FREQ_MIN_HZ) /* 4 800 000 */
#define PERIOD_MIN_TICKS (TIM_TICK_HZ / FREQ_MAX_HZ) /*    10 666 */

/* ── Счётчики ISR → main (volatile!) ── */
volatile uint32_t valid_pulse_count = 0;
volatile uint64_t sum_period_ticks = 0; /* uint64 на случай больших сумм */

#define MIN_PULSES_FOR_AVG 10 /* минимум для усреднения */
#define MAX_WINDOW_CALLS 10   /* макс. окно = 1 с */

void timer_100ms_callback(void)
{
    static float last_freq_hz = 0;
    static uint8_t counter = 0;
    counter++;

    __disable_irq();
    uint32_t cnt = valid_pulse_count;
    uint64_t sum = sum_period_ticks;
    __enable_irq();

    /* Ждём, пока наберётся достаточно импульсов ИЛИ истечёт время */
    if (cnt >= MIN_PULSES_FOR_AVG || counter >= MAX_WINDOW_CALLS)
    {
        /* Окно закрыто — считаем */
        __disable_irq();
        valid_pulse_count = 0;
        sum_period_ticks = 0;
        __enable_irq();
        counter = 0;
        float freq_hz = 0;
        if (cnt > 0)
        {
            freq_hz = TIM_TICK_HZ / ((float)sum / cnt);
        }
        last_freq_hz = freq_hz;
    }
    holdingRegisters[6].ActValue = sum;
    holdingRegisters[7].ActValue = cnt;
    modbusFloatToRegister(last_freq_hz, (uint16_t *)&holdingRegisters[8].ActValue);
}

void timer_10ms_callback(void)
{
    GEN_OUT_PORT->OUTDR ^= GEN_OUT_PIN;
}

void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_UP_IRQHandler()
{
    if (TIM1->INTFR & TIM_FLAG_Update)
    {
        TIM1->INTFR = ~TIM_FLAG_Update;
    }
}

void BoardTimer2Init(void)
{
    // Инициализация TIM2 Input Capture
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct = {0};
    TIM_ICInitTypeDef TIM_ICStruct = {0};
    NVIC_InitTypeDef NVIC_InitStruct = {0};

    /* 1. Тактирование GPIOA/GPIOD и TIM2 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* 2. PD4 → Floating Input (TIM2_CH1 / TI1) */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* 3. TimeBase: PSC=0 → 48 МГц, ARR=0xFFFF */
    TIM_TimeBaseStruct.TIM_Period = 0xFFFF;
    TIM_TimeBaseStruct.TIM_Prescaler = TIM_PRESCALER - 1;
    TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStruct);

    /* 4. CH1: Input Capture, TI1, rising edge, без делителя, фильтр N=8 */
    TIM_ICStruct.TIM_Channel = TIM_Channel_1;
    TIM_ICStruct.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICStruct.TIM_ICSelection = TIM_ICSelection_DirectTI; // CC1S = 01
    TIM_ICStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICStruct.TIM_ICFilter = 0x04; // N=8, fS=fDTS/2
    TIM_ICInit(TIM2, &TIM_ICStruct);

    /* 5. CH2: Input Capture, ТОЖЕ TI1 (cross-capture), falling edge */
    TIM_ICStruct.TIM_Channel = TIM_Channel_2;
    TIM_ICStruct.TIM_ICPolarity = TIM_ICPolarity_Falling;
    TIM_ICStruct.TIM_ICSelection = TIM_ICSelection_IndirectTI; // CC2S = 10 → TI1
    TIM_ICStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICStruct.TIM_ICFilter = 0x04;
    TIM_ICInit(TIM2, &TIM_ICStruct);

    /* 6. Разрешаем прерывания CC1, CC2, Update */
    TIM_ITConfig(TIM2, TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_Update, ENABLE);

    /* 7. NVIC */
    NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    /* 8. Старт таймера */
    TIM_Cmd(TIM2, ENABLE);
}

/* ── Файл-скоуп переменные ISR ── */
static volatile uint16_t prev_rising = 0;
static volatile uint32_t overflow_cnt_rising = 0;
static volatile uint32_t overflow_cnt_falling = 0;
static volatile uint32_t last_period_ticks = 0;
#define MAX_OVF_CNT 10000UL

void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void)
{
    /* ── Update ── */
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        if (overflow_cnt_rising < MAX_OVF_CNT)
            overflow_cnt_rising++;
        if (overflow_cnt_falling < MAX_OVF_CNT)
            overflow_cnt_falling++;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }

    /* ── CH1: rising ── */
    if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
    {
        uint16_t cap = TIM_GetCapture1(TIM2);
        last_period_ticks = (uint32_t)overflow_cnt_rising * 0x10000UL + (uint32_t)(cap - prev_rising);

        overflow_cnt_rising = 0;
        overflow_cnt_falling = 0;
        prev_rising = cap;

        TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
    }

    /* ── CH2: falling ── */
    if (TIM_GetITStatus(TIM2, TIM_IT_CC2) != RESET)
    {
        uint16_t cap = TIM_GetCapture2(TIM2);
        uint32_t pulse_ticks = (uint32_t)overflow_cnt_falling * 0x10000UL + (uint32_t)(cap - prev_rising);
        uint32_t period = last_period_ticks;

        /* ══════════════════════════════════════════════
         *  ФИЛЬТРАЦИЯ (целочисленная, без float!)
         * ══════════════════════════════════════════════ */
        uint8_t valid = 0;
        if (period != 0)
        {
            /* 1. Частота: PERIOD_MIN_TICKS ≤ period ≤ PERIOD_MAX_TICKS */
            if ((period >= PERIOD_MIN_TICKS) && (period <= PERIOD_MAX_TICKS))
            {
                /* 2. Скважность: 0.10 ≤ duty ≤ 0.90
                 *    duty = pulse/period
                 *    duty ≥ 0.10  ⇔  pulse * 10 ≥ period
                 *    duty ≤ 0.90  ⇔  pulse * 10 ≤ period * 9
                 */
                uint32_t p10 = pulse_ticks * 10UL;
                if ((p10 >= period) && (p10 <= period * DUTY_NUM_MAX))
                {
                    valid = 1;
                }
            }
        }

        /* ── Накопление для 100 мс задачи ── */
        if (valid)
        {
            valid_pulse_count++;
            sum_period_ticks += period;
        }

        TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);
    }
}