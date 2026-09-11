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
void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

volatile uint32_t tim_tick = 0;

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

    // Бесконечное переключение пина на максимальной скорости
    /* while (1)
    {
        GPIOD->BSHR = GPIO_Pin_3;
        GPIOD->BCR = GPIO_Pin_3;
    } */

    USART_Printf_Init(9600);
    printf("SystemClk:%d\r\n", SystemCoreClock);

    Delay_Init();

    InitModbus(MODBUS_ID);

    // 1ms tick
    // NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->SR &= ~(1 << 0);
    SysTick->CMP = (SystemCoreClock / 1000) - 1;
    SysTick->CNT = 0;
    SysTick->CTLR = 0xF;

    soft_timer_init(&test_timer, 3000, 0);

    while (1)
    {
        ProcessModbus();

        /* if (soft_timer_check(&test_timer))
        {
            // GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN);
            // save_Counter_To_Flash(0xAAAA);
            // GPIO_ResetBits(LED_GPIO_PORT, LED_GPIO_PIN);

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
        } */
    }

    return 0;
}

void USART1_IRQHandler(void)
{
    // receive interrupt
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t rx_data = USART_ReceiveData(USART1);
        // IMPORTANT: RS485 echo protection.
        // While we ourselves are transmitting (Tx_Index < Tx_Buf_Size), we will see our own bytes on RX.
        // We ignore them so as not to break the logic for parsing incoming frames.
        if (Tx_Index >= Tx_Buf_Size)
        {
            ReceiveInterrupt(rx_data);
        }
    }
    // tranceive interrupt
    if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
    {
        if (Tx_Index < Tx_Buf_Size)
        {
            // Put the next byte into the data register
            USART_SendData(USART1, Tx_Buf[Tx_Index++]);
        }
        else
        {
            // All bytes are loaded into the shift register.
            // Disable the TXE interrupt so it doesn't interfere.
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
            // We enable the TC interrupt to catch the moment the last byte is completely released.
            USART_ITConfig(USART1, USART_IT_TC, ENABLE);
        }
    }
    // DE switching
    if (USART_GetITStatus(USART1, USART_IT_TC) != RESET)
    {
        // Reset the TC flag (reading SR could already reset it, but to be safe)
        USART_ClearITPendingBit(USART1, USART_IT_TC);
        // Disable TC interrupt
        USART_ITConfig(USART1, USART_IT_TC, DISABLE);
        // Switch RS485 to receive (DE = LOW)
        set_rs485_de_disable();
        // Clear the buffer size, signaling the end of the transfer
        Tx_Buf_Size = 0;
        Tx_Index = 0;
    }
}

void TIM1_UP_IRQHandler()
{
    if (TIM1->INTFR & TIM_FLAG_Update)
    {
        tim_tick++;

        ModBus_TimerValues();

        if (tim_tick % 1000 == 0)
        {
            holdingRegisters[0].ActValue++;
            holdingRegisters[1].ActValue++;
            holdingRegisters[2].ActValue++;

            save_Counter_To_Flash(0xAAAA);
        }
        TIM1->INTFR = ~TIM_FLAG_Update;
    }
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
    // ModBus_TimerValues();
    // modbus_timeout_inc(10);
    SysTick->SR = 0;
}
