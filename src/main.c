#include "debug.h"
#include "Modbus.h"
#include "board.h"
#include "soft_timer.h"
#include "eeprom.h"

/* Global define */
#define MODBUS_ID 0x1

/* Global Variable */
soft_timer_t test_timer;
soft_timer_t timer_100ms;
volatile uint8_t aaa1 = 0;
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

    soft_timer_init(&test_timer, 1000, 0);
    soft_timer_init(&timer_100ms, 100, timer_100ms_callback);

    while (1)
    {
        ProcessModbus();
        soft_timer_check(&timer_100ms);

        if (soft_timer_check(&test_timer))
        {
            // save_Counter_To_Flash(0xAAAA);
            holdingRegisters[0].ActValue++;
            holdingRegisters[1].ActValue++;
            holdingRegisters[2].ActValue++;

            if (aaa1)
            {
                aaa1 = 0;
                GPIO_SetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);
            }
            else
            {
                aaa1 = 1;
                GPIO_ResetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);
            }
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
    soft_timer_inc(1);
    ModBus_TimerValues();
    SysTick->SR = 0;
}
