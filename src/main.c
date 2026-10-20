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
#include "interpolation.h"

/* Global define */
#define MODBUS_ID 0x1

/* Global Variable */
soft_timer_t test_timer;
soft_timer_t timer_100ms;
soft_timer_t timer_10ms;

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

    meterInit();

    SoftTimerInit(&test_timer, 500, 0);
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
