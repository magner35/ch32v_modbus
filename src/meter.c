/*
 * Created on Wed Sep 16 2026
 *
 * Copyright (c) 2026 by magner mr.jaedong@gmail.com
 */
#include "meter.h"
#include <stdint.h>
#include "debug.h"
#include "board.h"

volatile uint8_t aaa2 = 0;

void timer_100ms_callback(void)
{
    if (aaa2)
    {
        aaa2 = 0;
        GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);
    }
    else
    {
        aaa2 = 1;
        GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);
    }
}