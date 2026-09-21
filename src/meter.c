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

void timer_100ms_callback(void)
{
    // LED2_GPIO_PORT->OUTDR ^= LED2_GPIO_PIN;
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