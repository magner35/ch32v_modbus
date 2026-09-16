/*
 * Created on Wed Sep 16 2026
 *
 * Copyright (c) 2026 by magner mr.jaedong@gmail.com
 */

#include "meter.h"
#include <stdint.h>
#include "debug.h"
#include "board.h"

void timer_100ms_callback(void)
{
    LED2_GPIO_PORT->OUTDR ^= LED2_GPIO_PIN;
}