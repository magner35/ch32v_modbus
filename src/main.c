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

/* Global define */
#define MODBUS_ID 0x1

/* Global Variable */
soft_timer_t test_timer;
soft_timer_t timer_100ms;

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    BoardGpioInit();
    InitModbus(MODBUS_ID);

    // 1ms tick
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->SR &= ~(1 << 0);
    SysTick->CMP = (SystemCoreClock / 1000) - 1;
    SysTick->CNT = 0;
    SysTick->CTLR = 0xF;

    SoftTimerInit(&test_timer, 1000, 0);
    SoftTimerInit(&timer_100ms, 100, timer_100ms_callback);

    while (1)
    {
        ProcessModbus();

        SoftTimerCheck(&timer_100ms);

        if (SoftTimerCheck(&test_timer))
        {
            // save_Counter_To_Flash(0xAAAA);
            holdingRegisters[0].ActValue++;
            holdingRegisters[1].ActValue = sizeof(char);
            holdingRegisters[2].ActValue = sizeof(int);
            holdingRegisters[3].ActValue = sizeof(long);
            holdingRegisters[4].ActValue = sizeof(float);
            holdingRegisters[5].ActValue = sizeof(double);
            holdingRegisters[6].ActValue = sizeof(bool);
            holdingRegisters[7].ActValue = sizeof(short);
            holdingRegisters[8].ActValue = sizeof(uint64_t);
            holdingRegisters[9].ActValue = sizeof(long long);

            inputRegisters[0].ActValue++;
            inputRegisters[1].ActValue--;
            inputRegisters[2].ActValue++;
            inputRegisters[3].ActValue--;
            inputRegisters[4].ActValue++;
            inputRegisters[5].ActValue--;
            inputRegisters[6].ActValue++;
            inputRegisters[7].ActValue--;
            inputRegisters[8].ActValue++;
            inputRegisters[9].ActValue--;

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
