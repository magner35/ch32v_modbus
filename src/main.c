#include "debug.h"
#include "Modbus.h"
#include "soft_timer.h"
#include "eeprom.h"

/* Global define */
#define MODBUS_ID 0x1

// Output signal
#define LED_GPIO_PORT (GPIOD)
#define LED_GPIO_PIN (GPIO_Pin_3)

/* Global Variable */
soft_timer_t test_timer;
volatile uint8_t aaa = 0;

/* interrupts */

int main(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();

    // LED
    GPIO_InitStructure.GPIO_Pin = LED_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);

    Delay_Init();
    InitModbus(MODBUS_ID);

    // 1ms tick
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->SR &= ~(1 << 0);
    SysTick->CMP = (SystemCoreClock / 1000) - 1;
    SysTick->CNT = 0;
    SysTick->CTLR = 0xF;

    soft_timer_init(&test_timer, 1000, 0);

    while (1)
    {
        ProcessModbus();
        if (soft_timer_check(&test_timer))
        {
            // save_Counter_To_Flash(0xAAAA);

            holdingRegisters[0].ActValue++;
            holdingRegisters[1].ActValue++;
            holdingRegisters[2].ActValue++;

            if (aaa)
            {
                aaa = 0;
                GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN);
            }
            else
            {
                aaa = 1;
                GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN);
            }
        }
    }

    return 0;
}

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
