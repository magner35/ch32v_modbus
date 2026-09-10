#include "debug.h"
#include "Modbus.h"
#include "soft_timer.h"

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
    Delay_Init();

    InitModbus(MODBUS_ID);

    // LED
    GPIO_InitStructure.GPIO_Pin = LED_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);

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

void USART1_IRQHandler_old(void)
{
    // receive interrupt
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        ReceiveInterrupt(USART_ReceiveData(USART1));
    }
}

void USART1_IRQHandler(void)
{
    // receive interrupt
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t rx_data = USART_ReceiveData(USART1);
        // ВАЖНО: Защита от эха RS485.
        // Пока мы сами передаем (Tx_Index < Tx_Buf_Size), мы будем видеть свои же байты на RX.
        // Игнорируем их, чтобы не сломать логику парсинга входящих кадров.
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
            // Кладем следующий байт в регистр данных
            USART_SendData(USART1, Tx_Buf[Tx_Index++]);
        }
        else
        {
            // Все байты загружены в сдвиговый регистр.
            // Отключаем прерывание TXE, чтобы не мешало.
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
            // Включаем прерывание TC, чтобы поймать момент полного выхода последнего байта.
            USART_ITConfig(USART1, USART_IT_TC, ENABLE);
        }
    }
    // DE switching
    if (USART_GetITStatus(USART1, USART_IT_TC) != RESET)
    {
        // Сбрасываем флаг TC (чтение SR уже могло его сбросить, но для надежности)
        USART_ClearITPendingBit(USART1, USART_IT_TC);
        // Отключаем прерывание TC
        USART_ITConfig(USART1, USART_IT_TC, DISABLE);
        // Переключаем RS485 на прием (DE = LOW)
        set_rs485_de_disable();
        // Очищаем размер буфера, сигнализируя об окончании передачи
        Tx_Buf_Size = 0;
        Tx_Index = 0;
    }
}

void TIM1_UP_IRQHandler()
{
    if (TIM1->INTFR & TIM_FLAG_Update)
    {
        tim_tick++;
        if (tim_tick % 1000 == 0)
        {
            holdingRegisters[0].ActValue++;
            holdingRegisters[1].ActValue++;
            holdingRegisters[2].ActValue++;
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
    ModBus_TimerValues();
    // modbus_timeout_inc(10);
    SysTick->SR = 0;
}
