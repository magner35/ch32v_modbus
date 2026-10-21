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
#include "filter.h"
#include "Modbus.h"
#include "modbus_registers.h"
#include "settings.h"
#include "eeprom.h"
#include "powermon.h"

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
/* Сумма периодов за окно ≤ 1 с ≈ 48 млн тиков + один период — uint32
 * хватает с запасом, а float-конверсия uint64 тянула ~300 Б библиотеки */
volatile uint32_t sum_period_ticks = 0;

/* Наработка прибора, сек: инкремент в timer_100ms_callback, восстановление
 * из flash при старте, сохранение при аварии питания */
uint32_t runtime_device;

#define MAX_WINDOW_CALLS 10 /* макс. окно = 1 с */

// constant for converting milliliters to nanoliters
#define ML_TO_NL_FACTOR 1000000.0F

extern ema_filter_t ema_rate_filter;

void timer_100ms_callback(void)
{
    static uint8_t i = 0;
    static float last_freq_hz = 0;
    static uint8_t counter = 0;
    static uint32_t impulses = 0;
    static uint8_t linearisation_points = 0;
    static int64_t totalnl = 0;
    static int64_t gtotalnl = 0;

    float kf_value;
    float rateavg = 0;
    float rateavg_prev = 0;
    float rate;

    counter++;

    __disable_irq();
    uint32_t cnt = valid_pulse_count;
    uint32_t sum = sum_period_ticks;
    __enable_irq();

    /* Ждём, пока наберётся достаточно импульсов ИЛИ истечёт время */
    if (cnt >= g_settings.rate_averaging || counter >= MAX_WINDOW_CALLS)
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
        impulses = impulses + cnt;
    }

    /* раз в секунду: наработка + пересчёт таблицы линеаризации */
    if (++i == 10)
    {
        i = 0;
        runtime_device++;
        linearisation_points = settingsLpoints();
    }

    GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);
    if (g_settings.linearisation)
    {
        kf_value = interpolationConstrainedSpline(lpoint_forward_frequency, lpoint_forward_value, linearisation_points, last_freq_hz, 1); // 690 us
    }
    else
        kf_value = g_settings.multiplier_value_average;
    GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);

    // rate calculation
    float rate_raw = kf_value * last_freq_hz * COEF_RATE();
    float rateMLPM = kf_value * last_freq_hz * COEF_RATELPM(); // unsigned

    if (g_settings.rate_damping != 0)
        rate = emaFilterUpdate(&ema_rate_filter, rate_raw);
    else
        rate = rate_raw; // unsigned

    // total pack calculation
    // calculate nanoliters per pulse (once per packet)
    float nl_per_pulse = kf_value * COEF_KF() * ML_TO_NL_FACTOR;
    // multiply the integer number of pulses by this factor and round it up
    uint64_t total_packetnl = (uint64_t)((float)cnt * nl_per_pulse + 0.5F);
    totalnl += total_packetnl;
    gtotalnl += total_packetnl;
    // Transferring data to the ml variables
    float totalml = (float)totalnl / ML_TO_NL_FACTOR;
    float gtotalml = (float)gtotalnl / ML_TO_NL_FACTOR;
    // Recalculating the total, taking into account the fact that there are now nanoliters inside
    float total = ((float)totalnl / ML_TO_NL_FACTOR) / COEF_TOTAL();
    float gtotal = (float)gtotalnl / (ML_TO_NL_FACTOR * COEF_TOTAL());

    modbusFloatToRegister(rate, &inputRegisters[IREG_RATE]);
    modbusFloatToRegister(total, &inputRegisters[IREG_TOTAL]);
    modbusFloatToRegister(gtotal, &inputRegisters[IREG_GTOTAL]);

    // modbusFloatToRegister(gtotal, &inputRegisters[IREG_GTOTAL]);
    // modbusFloatToRegister(gtotal, &inputRegisters[IREG_GTOTAL]);
    // modbusFloatToRegister(gtotal, &inputRegisters[IREG_GTOTAL]);

    modbusFloatToRegister(last_freq_hz, &inputRegisters[IREG_FREQUENCY]);
    modbusLongToRegister(impulses, &inputRegisters[IREG_IMPULSES]);
    modbusLongToRegister(runtime_device, &inputRegisters[IREG_RUNTIME_DEVICE]);

    /* отладочный регистр: младший байт — журнал критических данных
     * (бит0 найден, бит1 записан, бит2 верифицирован, бит3 прошлая сессия
     * с PVD-сохранением, бит4 аварийный путь, бит5 запись легла, бит6 НЕ
     * легла); биты 8-9 — PVDO при старте / PVD-прерывание;
     * биты 12-15 — счётчик входов в аварию */
    inputRegisters[IREG_DEBUG1] = (uint16_t)(critical_debug | ((uint16_t)powermon_debug << 8) | ((uint16_t)(powermon_fail_events & 0x0F) << 12));
    inputRegisters[IREG_DEBUG2] = linearisation_points;
    modbusFloatToRegister(kf_value, &inputRegisters[IREG_DEBUG3]);
    inputRegisters[IREG_DEBUG5] = 1979;
}

void timer_10ms_callback(void)
{
    GEN_OUT_PORT->OUTDR ^= GEN_OUT_PIN;
}

/* Прежняя реализация на SPL — оставлена для справки/отката, в прошивку
 * не линкуется (нет вызовов, --gc-sections убирает). Развернуть при
 * необходимости: заменить имя обратно и удалить регистровую версию. */
void BoardTimer2Init_SPL(void)
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

/* Инициализация TIM2 Input Capture — прямые записи в регистры.
 * Эквивалент BoardTimer2Init_SPL. Экономит ~390 Б flash (TIM_ICInit,
 * TIM_SetICxPrescaler, GPIO_Init, TIM_TimeBaseInit, TIM_Cmd/ITConfig,
 * RCC_APBxPeriphClockCmd). */
void BoardTimer2Init(void)
{
    /* 1. Тактирование TIM2 (APB1) и GPIOD (APB2) */
    RCC->APB1PCENR |= RCC_APB1Periph_TIM2;
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;

    /* 2. PD4 → вход с подтяжкой вверх: CNF=10, MODE=00, ODR=1 */
    GPIOD->CFGLR = (GPIOD->CFGLR & ~((uint32_t)0xFu << 16)) | (0x8u << 16);
    GPIOD->OUTDR |= GPIO_Pin_4;

    /* 3. TimeBase: PSC=0 (48 МГц), ARR=0xFFFF, счёт вверх */
    TIM2->PSC = TIM_PRESCALER - 1;
    TIM2->ATRLR = 0xFFFF;
    TIM2->SWEVGR = TIM_UG; /* перезагрузка прескалера */

    /* 4. Захват: CH1 — TI1 по фронту (CC1S=01, IC1F=0100 → N=8),
     *    CH2 — тот же TI1 по срезу (CC2S=10, IC2F=0100) */
    TIM2->CHCTLR1 = (uint16_t)(TIM_IC2F_1 | TIM_IC2F_2 /* IC2F=0100 */ |
                                (TIM_CC2S_1)            /* CC2S=10, косвенно TI1 */ |
                                TIM_IC1F_1 | TIM_IC1F_2 /* IC1F=0100 */ |
                                TIM_CC1S_0);            /* CC1S=01, напрямую TI1 */
    /* полярность: CC1E=1/CC1P=0 (фронт), CC2E=1/CC2P=1 (срез) */
    TIM2->CCER = (uint16_t)(TIM_CC1E | TIM_CC2E | TIM_CC2P);

    /* 5. Прерывания: CC1, CC2, Update */
    TIM2->DMAINTENR = TIM_UIE | TIM_CC1IE | TIM_CC2IE;

    /* 6. NVIC: эквивалент NVIC_Init {preemption 1, sub 1} в группе 1:
     * байт приоритета = (1<<7) | (1<<6) = 0xC0 */
    NVIC_SetPriority(TIM2_IRQn, 0xC0);
    NVIC_EnableIRQ(TIM2_IRQn);

    /* 7. Сброс флагов и старт */
    TIM2->INTFR = (uint16_t)~(TIM_IT_Update | TIM_IT_CC1 | TIM_IT_CC2);
    TIM2->CTLR1 = TIM_CEN; /* счёт вверх (DIR=0 по умолчанию) */
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