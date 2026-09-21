#include "ModbusPort.h"
#include "debug.h"

// Modbus RTU Variables
volatile unsigned char ReceiveBuffer[MODBUS_RECEIVE_BUFFER_SIZE]; // Buffer to collect data from hardware
volatile unsigned char ReceiveCounter = 0;                        // Collected data number

// UART Initialize
void ModBus_UART_Initialise(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    /* USART1 TX-->D.5   RX-->D.6 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitTypeDef USART_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;

    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Serial port enable output
    GPIO_InitStructure.GPIO_Pin = RS485_DE_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RS485_DE_GPIO_PORT, &GPIO_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

/*************************Interrupt Fonction Slave*****************************/
// Call this function into your UART Interrupt. Collect data from it!
// Better to use DMA
void ReceiveInterrupt(unsigned char Data)
{
    ReceiveBuffer[ReceiveCounter] = Data;
    ReceiveCounter++;

    if (ReceiveCounter > MODBUS_RECEIVE_BUFFER_SIZE)
        ReceiveCounter = 0;

    ModbusTimerValue = 0;
}

// Call this function into 1ms Interrupt or Event!
void ModBus_TimerValues(void)
{
    ModbusTimerValue++;
    Tx_DelayCounter++;
}

void set_rs485_de_enable(void)
{
    GPIO_SetBits(RS485_DE_GPIO_PORT, RS485_DE_GPIO_PIN);
}

void set_rs485_de_disable(void)
{
    GPIO_ResetBits(RS485_DE_GPIO_PORT, RS485_DE_GPIO_PIN);
}

void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
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

/******************************************************************************/
