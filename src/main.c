/*
 * Created on Wed Sep 16 2026
 *
 * Copyright (c) 2026 by magner mr.jaedong@gmail.com
 */

#include "debug.h"
#include "Modbus.h"
#include "ModbusPort.h"
#include "modbus_registers.h"
#include "board.h"
#include "soft_timer.h"
#include "eeprom.h"
#include "meter.h"
#include "interpolation_int.h"
#include "interpolation.h"

/* Global define */
#define MODBUS_ID 0x1

/* Global Variable */
soft_timer_t test_timer;
soft_timer_t timer_100ms;
soft_timer_t timer_10ms;

const float xValues[] = {10, 20, 40, 80, 160, 320};
const float yValues[] = {1, 2, 3, 4, 5, 6};

const int64_t xValues_int[] = {10, 20, 40, 80, 160, 320};
const int64_t yValues_int[] = {1000000, 2000000, 3000000, 4000000, 5000000, 6000000};

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    BoardGpioInit();

    BoardTimer1Init();
    BoardTimer2Init();

    InitModbus(MODBUS_ID);

    // 1ms tick
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->SR &= ~(1 << 0);
    SysTick->CMP = (SystemCoreClock / 1000) - 1;
    SysTick->CNT = 0;
    SysTick->CTLR = 0xF;

    SoftTimerInit(&test_timer, 100, 0);
    SoftTimerInit(&timer_100ms, 100, timer_100ms_callback);
    SoftTimerInit(&timer_10ms, 10, timer_10ms_callback);

    while (1)
    {
        ProcessModbus();

        SoftTimerCheck(&timer_100ms);
        SoftTimerCheck(&timer_10ms);

        if (SoftTimerCheck(&test_timer))
        {
            // save_Counter_To_Flash(0xAAAA);

            float result_float = 0;
            uint64_t result_int = 0;
            unsigned int linearisation_points = 6;
            unsigned int linearisation_predict = 0;
            float frequency = 30.0;
            int frequency_int = frequency;
            GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);

            // result_int = interpolationLinearInt64(xValues_int, yValues_int, linearisation_points, frequency_int, linearisation_predict); // 25 us
            // result_int = interpolationCatmullSplineInt64(xValues_int, yValues_int, linearisation_points, frequency_int, linearisation_predict); // 220 us
            // result_int = interpolationConstrainedSplineInt64(xValues_int, yValues_int, linearisation_points, frequency_int, linearisation_predict); // 404 us

            result_int = 12345678901234567890ULL;
            result_float = 123.456;

            // result_float = interpolationLinear(xValues, yValues, linearisation_points, frequency, linearisation_predict); // 60 us
            // result_float = interpolationCatmullSpline(xValues, yValues, linearisation_points, frequency, linearisation_predict); // 256 us
            // result_float = interpolationConstrainedSpline(xValues, yValues, linearisation_points, frequency, linearisation_predict); // 690 us
            GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN);

            modbusLLToRegister(result_int, (uint16_t *)&holdingRegisters[0].ActValue);

            modbusFloatToRegister_(result_float, (uint16_t *)&holdingRegisters[4].ActValue);

            LED1_GPIO_PORT->OUTDR ^= LED1_GPIO_PIN;
        }
    }

    return 0;
}

/* interrupts */

/*******************************************************************************
 * @brief System tick interrupt
 * @param None
 * @return None
 ******************************************************************************/
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    SoftTimerInc(1);
    ModBus_TimerValues();
    SysTick->SR = 0;
}
