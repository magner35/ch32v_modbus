#include "ModbusPort.h"
#include "debug.h"
#include "settings.h"

// Modbus RTU Variables
volatile unsigned char ReceiveBuffer[MODBUS_RECEIVE_BUFFER_SIZE]; // Buffer to collect data from hardware
volatile unsigned char ReceiveCounter = 0;                        // Collected data number

/* Прежняя реализация на SPL — оставлена для справки/отката, в прошивку
 * не линкуется (нет вызовов). Вернуть: переименовать обратно. */
void ModBus_UART_Initialise_SPL(void)
{
    /* Параметры интерфейса — из настроек (ЗОВЁТСЯ СТРОГО ПОСЛЕ meterInit!).
     * Значения enum'ов см. SETTINGS_TABLE: baudrate 0-9, parity 0-2, stop 0/1 */
    static const uint32_t baud_table[] = {1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200};
    uint32_t baud = baud_table[g_settings.interfaceRS485_baudrate % (sizeof(baud_table) / sizeof(baud_table[0]))];
    uint16_t parity = USART_Parity_No;
    uint16_t word_length = USART_WordLength_8b;

    switch (g_settings.interfaceRS485_parity)
    {
    case 1:
        parity = USART_Parity_Even;
        word_length = USART_WordLength_9b; /* 8 данных + бит чётности */
        break;
    case 2:
        parity = USART_Parity_Odd;
        word_length = USART_WordLength_9b;
        break;
    default:
        break;
    }

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

    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = word_length;
    USART_InitStructure.USART_StopBits = (g_settings.interfaceRS485_stopbits)
                                             ? USART_StopBits_2
                                             : USART_StopBits_1;
    USART_InitStructure.USART_Parity = parity;
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

/* Инициализация USART1 (RS-485) — прямые записи в регистры.
 * Эквивалент ModBus_UART_Initialise_SPL, экономит ~360 Б flash
 * (USART_Init, RCC_GetClocksFreq, GPIO_Init, NVIC_Init).
 * Параметры — из настроек, ЗОВЁТСЯ СТРОГО ПОСЛЕ meterInit! */
void ModBus_UART_Initialise(void)
{
    static const uint32_t baud_table[] = {1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200};
    uint32_t baud = baud_table[g_settings.interfaceRS485_baudrate % (sizeof(baud_table) / sizeof(baud_table[0]))];

    /* CTLR1: UE + RXNEIE + TE + RE; при чётности — 9-битное слово
     * (8 данных + бит чётности), PS=1 для нечётной */
    uint16_t ctlr1 = USART_CTLR1_UE | USART_CTLR1_RXNEIE | USART_Mode_Tx | USART_Mode_Rx;
    switch (g_settings.interfaceRS485_parity)
    {
    case 1:
        ctlr1 |= USART_WordLength_9b | USART_Parity_Even;
        break;
    case 2:
        ctlr1 |= USART_WordLength_9b | USART_Parity_Odd;
        break;
    default:
        break;
    }
    uint16_t ctlr2 = (g_settings.interfaceRS485_stopbits) ? USART_StopBits_2 : 0;

    /* Тактирование: GPIOD/GPIOC (APB2), USART1 (APB2) */
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_USART1;

    /* PD5 (TX) — альтернативная функция push-pull: CNF=10,MODE=11 (50 МГц)
     * PD6 (RX) — вход плавающий: CNF=01,MODE=00
     * PC3 (DE) — выход push-pull 50 МГц: CNF=00,MODE=11 */
    GPIOD->CFGLR = (GPIOD->CFGLR & ~((uint32_t)0xFu << 20)) | (0xBu << 20);
    GPIOD->CFGLR = (GPIOD->CFGLR & ~((uint32_t)0xFu << 24)) | (0x4u << 24);
    GPIOC->CFGLR = (GPIOC->CFGLR & ~((uint32_t)0xFu << 12)) | (0x3u << 12);
    set_rs485_de_disable(); /* приём по умолчанию */

    /* USART1: PCLK2 = SystemCoreClock (48 МГц) */
    USART1->CTLR2 = ctlr2;
    USART1->CTLR3 = 0; /* без аппаратного управления потоком */
    USART1->BRR = (SystemCoreClock + baud / 2) / baud;
    USART1->CTLR1 = ctlr1;

    /* приоритет как у SPL-версии: preemption 1, sub 1 → 0xC0 */
    NVIC_SetPriority(USART1_IRQn, 0xC0);
    NVIC_EnableIRQ(USART1_IRQn);
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
