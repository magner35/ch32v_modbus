#include "Modbus.h"
#include "debug.h"
#include "ModbusPort.h"

/*******************************ModBus Functions*******************************/
#define MODBUS_READ_COILS 1
#define MODBUS_READ_DISCRETE_INPUTS 2
#define MODBUS_READ_HOLDING_REGISTERS 3
#define MODBUS_READ_INPUT_REGISTERS 4
#define MODBUS_WRITE_SINGLE_COIL 5
#define MODBUS_WRITE_SINGLE_REGISTER 6
#define MODBUS_WRITE_MULTIPLE_COILS 15
#define MODBUS_WRITE_MULTIPLE_REGISTERS 16
/****************************End of ModBus Functions***************************/

#define FALSE_FUNCTION 0
#define FALSE_SLAVE_ADDRESS 1
#define DATA_NOT_READY 2
#define DATA_READY 3

#define ERROR_CODE_01 0x01 // Function code is not supported
#define ERROR_CODE_02 0x02 // Register address is not allowed or write-protected

unsigned char MODBUS_SLAVE_ADDRESS = 1;

typedef enum
{
    RXTX_IDLE,
    RXTX_START,
    RXTX_DATABUF,
    RXTX_WAIT_ANSWER,
    RXTX_TIMEOUT
} RXTX_STATE;

/* Два буфера вместо четырёх: запрос разбирается на месте из ReceiveBuffer
 * (ModbusPort), ответ строится прямо в Tx_Buf: [0]=адрес, [1]=функция,
 * далее данные; Tx_Len — длина ответа без CRC, CRC дописывает TxRTU. */
volatile unsigned char Tx_Buf[MODBUS_TRANSMIT_BUFFER_SIZE];
volatile uint8_t Tx_Buf_Size = 0;
volatile uint8_t Tx_Index = 0;
volatile uint8_t Tx_DelayCounter = 0;
static uint8_t Tx_Len = 0;      /* длина ответа без CRC */
static unsigned int Tx_CRC16 = 0xFFFF;
static RXTX_STATE Tx_State = RXTX_IDLE;

static unsigned char Rx_Function = 0;
static unsigned char Rx_DataLen = 0; /* байт данных в запросе без адр/фц/CRC */
static unsigned int Rx_CRC16 = 0xFFFF;
static RXTX_STATE Rx_State = RXTX_IDLE;
static unsigned char Rx_Data_Available = FALSE;

volatile unsigned short ModbusTimerValue = 0;

uint16_t holdingRegisters[NUMBER_OF_HOLDING_REGISTERS];
uint16_t inputRegisters[NUMBER_OF_INPUT_REGISTERS];

/****************End of Slave Transmit and Receive Variables*******************/

/*
 * Function Name        : CRC16
 * @param[in]           : Data  - Data to Calculate CRC
 * @param[in/out]       : CRC   - Instant CRC value
 * @How to use          : First initial data has to be 0xFFFF.
 */
void CRC16(const unsigned char Data, unsigned int *CRC)
{
    unsigned int i;

    *CRC = *CRC ^ (unsigned int)Data;
    for (i = 8; i > 0; i--)
    {
        if (*CRC & 0x0001)
            *CRC = (*CRC >> 1) ^ 0xA001;
        else
            *CRC >>= 1;
    }
}
/******************************************************************************/

/*
 * Function Name        : DoTx
 * @param[out]          : TRUE
 * @How to use          : It is used for send data package over physical layer
 */
unsigned char DoSlaveTX(void)
{
    set_rs485_de_enable();
    Tx_Index = 0;
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
    return TRUE;
}
/******************************************************************************/

/*
 * Function Name        : SendMessage
 * @param[out]          : TRUE/FALSE
 * @How to use          : This function start to sending messages
 */
unsigned char SendMessage(void)
{
    if (Tx_State != RXTX_IDLE)
        return FALSE;
    Tx_DelayCounter = 0;
    Tx_State = RXTX_START;
    return TRUE;
}
/******************************************************************************/

/*
 * Function Name        : HandleModbusError
 * @How to use          : This function generated errors to Modbus Master
 */
void HandleModbusError(char ErrorCode)
{
    Tx_Buf[0] = MODBUS_SLAVE_ADDRESS;
    Tx_Buf[1] = Rx_Function | 0x80;
    Tx_Buf[2] = (unsigned char)ErrorCode;
    Tx_Len = 3;
    SendMessage();
}
/******************************************************************************/

/*
 * Function Name        : HandleModbusReadInputRegisters
 * @How to use          : Modbus function 04 - Read input registers
 */
#if MODBUS_READ_INPUT_REGISTERS_ENABLED > 0
void HandleModbusReadInputRegisters(void)
{
    unsigned int StartAddress = ((unsigned int)ReceiveBuffer[2] << 8) + ReceiveBuffer[3];
    unsigned int NumberOfRegisters = ((unsigned int)ReceiveBuffer[4] << 8) + ReceiveBuffer[5];

    if ((StartAddress + NumberOfRegisters) > NUMBER_OF_INPUT_REGISTERS)
    {
        HandleModbusError(ERROR_CODE_02);
    }
    else
    {
        Tx_Buf[0] = MODBUS_SLAVE_ADDRESS;
        Tx_Buf[1] = MODBUS_READ_INPUT_REGISTERS;
        Tx_Buf[2] = (unsigned char)(NumberOfRegisters * 2);
        for (unsigned int i = 0; i < NumberOfRegisters; i++)
        {
            unsigned short CurrentData = inputRegisters[StartAddress + i];
            Tx_Buf[3 + 2 * i] = (unsigned char)((CurrentData & 0xFF00) >> 8);
            Tx_Buf[4 + 2 * i] = (unsigned char)(CurrentData & 0xFF);
        }
        Tx_Len = 3 + 2 * NumberOfRegisters;
        SendMessage();
    }
}
#endif
/******************************************************************************/

/*
 * Function Name        : HandleModbusReadHoldingRegisters
 * @How to use          : Modbus function 03 - Read holding registers
 */
#if MODBUS_READ_HOLDING_REGISTERS_ENABLED > 0
void HandleModbusReadHoldingRegisters(void)
{
    unsigned int StartAddress = ((unsigned int)ReceiveBuffer[2] << 8) + ReceiveBuffer[3];
    unsigned int NumberOfRegisters = ((unsigned int)ReceiveBuffer[4] << 8) + ReceiveBuffer[5];

    if ((StartAddress + NumberOfRegisters) > NUMBER_OF_HOLDING_REGISTERS)
    {
        HandleModbusError(ERROR_CODE_02);
    }
    else
    {
        Tx_Buf[0] = MODBUS_SLAVE_ADDRESS;
        Tx_Buf[1] = MODBUS_READ_HOLDING_REGISTERS;
        Tx_Buf[2] = (unsigned char)(NumberOfRegisters * 2);
        for (unsigned int i = 0; i < NumberOfRegisters; i++)
        {
            unsigned short CurrentData = holdingRegisters[StartAddress + i];
            Tx_Buf[3 + 2 * i] = (unsigned char)((CurrentData & 0xFF00) >> 8);
            Tx_Buf[4 + 2 * i] = (unsigned char)(CurrentData & 0xFF);
        }
        Tx_Len = 3 + 2 * NumberOfRegisters;
        SendMessage();
    }
}
#endif
/******************************************************************************/

/*
 * Function Name        : HandleModbusWriteSingleRegister
 * @How to use          : Modbus function 06 - Write single register
 */
#if MODBUSWRITE_SINGLE_REGISTER_ENABLED > 0
void HandleModbusWriteSingleRegister(void)
{
    unsigned int Address = ((unsigned int)ReceiveBuffer[2] << 8) + ReceiveBuffer[3];
    unsigned int Value = ((unsigned int)ReceiveBuffer[4] << 8) + ReceiveBuffer[5];

    Tx_Buf[0] = MODBUS_SLAVE_ADDRESS;
    Tx_Buf[1] = MODBUS_WRITE_SINGLE_REGISTER;
    if (Address >= NUMBER_OF_HOLDING_REGISTERS)
    {
        Tx_Buf[1] |= 0x80;
        Tx_Buf[2] = ERROR_CODE_02;
        Tx_Len = 3;
    }
    else
    {
        holdingRegisters[Address] = (uint16_t)Value;
        /* ответ — эхо запроса */
        for (unsigned char i = 0; i < 4; ++i)
            Tx_Buf[2 + i] = ReceiveBuffer[2 + i];
        Tx_Len = 6;
    }
    SendMessage();
}
#endif
/******************************************************************************/

/*
 * Function Name        : HandleModbusWriteMultipleRegisters
 * @How to use          : Modbus function 16 - Write multiple registers
 */
#if MODBUS_WRITE_MULTIPLE_REGISTERS_ENABLED > 0
void HandleModbusWriteMultipleRegisters(void)
{
    unsigned int StartAddress = ((unsigned int)ReceiveBuffer[2] << 8) + ReceiveBuffer[3];
    unsigned int NumberOfRegisters = ((unsigned int)ReceiveBuffer[4] << 8) + ReceiveBuffer[5];

    Tx_Buf[0] = MODBUS_SLAVE_ADDRESS;
    Tx_Buf[1] = MODBUS_WRITE_MULTIPLE_REGISTERS;
    if ((StartAddress + NumberOfRegisters) > NUMBER_OF_HOLDING_REGISTERS)
    {
        Tx_Buf[1] |= 0x80;
        Tx_Buf[2] = ERROR_CODE_02;
        Tx_Len = 3;
    }
    else
    {
        for (unsigned int i = 0; i < NumberOfRegisters; i++)
        {
            unsigned int Value = ((unsigned int)ReceiveBuffer[7 + 2 * i] << 8) + ReceiveBuffer[8 + 2 * i];
            holdingRegisters[StartAddress + i] = (uint16_t)Value;
        }
        /* ответ — эхо адреса и количества */
        for (unsigned char i = 0; i < 4; ++i)
            Tx_Buf[2 + i] = ReceiveBuffer[2 + i];
        Tx_Len = 6;
    }
    SendMessage();
}
#endif
/******************************************************************************/

/*
 * Function Name        : RxDataAvailable
 * @return              : If Data is Ready, Return TRUE
 *                        If Data is not Ready, Return FALSE
 */
unsigned char RxDataAvailable(void)
{
    unsigned char Result = Rx_Data_Available;
    Rx_Data_Available = FALSE;
    return Result;
}
/******************************************************************************/

/*
 * Function Name        : CheckRxTimeout
 * @return              : If Time is out return TRUE
 *                        If Time is not out return FALSE
 */
unsigned char CheckRxTimeout(void)
{
    // A return value of true indicates there is a timeout
    if (ModbusTimerValue >= MODBUS_TIMEOUTTIMER)
    {
        ModbusTimerValue = 0;
        ReceiveCounter = 0;
        return TRUE;
    }
    return FALSE;
}
/******************************************************************************/

/*
 * Function Name        : CheckBufferComplete
 * @return              : If data is ready, return              DATA_READY
 *                        If slave address is wrong, return     FALSE_SLAVE_ADDRESS
 *                        If data is not ready, return          DATA_NOT_READY
 *                        If functions is wrong, return         FALSE_FUNCTION
 */
unsigned char CheckModbusBufferComplete(void)
{
    int ExpectedReceiveCount = 0;
    if (ReceiveCounter > 4)
    {
        if (ReceiveBuffer[0] == MODBUS_SLAVE_ADDRESS)
        {
            if (ReceiveBuffer[1] == 0x01 || ReceiveBuffer[1] == 0x02 || ReceiveBuffer[1] == 0x03 || ReceiveBuffer[1] == 0x04 || ReceiveBuffer[1] == 0x05 || ReceiveBuffer[1] == 0x06) // RHR
            {
                ExpectedReceiveCount = 8;
            }
            else if (ReceiveBuffer[1] == 0x0F || ReceiveBuffer[1] == 0x10)
            {
                ExpectedReceiveCount = ReceiveBuffer[6] + 9;
            }
            else
            {
                ReceiveCounter = 0;
                return FALSE_FUNCTION;
            }
        }
        else
        {
            ReceiveCounter = 0;
            return FALSE_SLAVE_ADDRESS;
        }
    }
    else
        return DATA_NOT_READY;
    if (ReceiveCounter == ExpectedReceiveCount)
    {
        return DATA_READY;
    }
    return DATA_NOT_READY;
}
/******************************************************************************/

/*
 * Function Name        : RxRTU
 * @How to use          : Check for data ready, if it is good return answer
 */
void RxRTU(void)
{
    unsigned char i;
    unsigned char ReceiveBufferControl = 0;
    ReceiveBufferControl = CheckModbusBufferComplete();
    if (ReceiveBufferControl == DATA_READY)
    {
        /* запрос разбирается на месте из ReceiveBuffer */
        Rx_CRC16 = 0xffff;
        CRC16(ReceiveBuffer[0], &Rx_CRC16);
        Rx_Function = ReceiveBuffer[1];
        CRC16(Rx_Function, &Rx_CRC16);
        Rx_DataLen = ReceiveCounter - 4; /* данные без адреса, функции и CRC */
        Rx_State = RXTX_DATABUF;
        ReceiveCounter = 0;
    }
    CheckRxTimeout();
    if ((Rx_State == RXTX_DATABUF) && (Rx_DataLen >= 2))
    {
        // Finish off our CRC check
        unsigned char dataLen = Rx_DataLen;
        for (i = 0; i < dataLen; ++i)
        {
            CRC16(ReceiveBuffer[2 + i], &Rx_CRC16);
        }
        if (((unsigned int)ReceiveBuffer[2 + dataLen] + ((unsigned int)ReceiveBuffer[3 + dataLen] << 8)) == Rx_CRC16)
        {
            // Valid message!
            Rx_Data_Available = TRUE;
        }
        Rx_State = RXTX_IDLE;
    }
}
/******************************************************************************/

/*
 * Function Name        : TxRTU
 * @How to use          : If it is ready send answers!
 */
void TxRTU(void)
{
    Tx_CRC16 = 0xFFFF;
    for (unsigned int i = 0; i < Tx_Len; i++)
    {
        CRC16(Tx_Buf[i], &Tx_CRC16);
    }
    Tx_Buf[Tx_Len] = Tx_CRC16 & 0x00FF;
    Tx_Buf[Tx_Len + 1] = (Tx_CRC16 & 0xFF00) >> 8;
    Tx_Buf_Size = Tx_Len + 2;
    DoSlaveTX();
    Tx_State = RXTX_IDLE;
}
/******************************************************************************/

/*
 * Function Name        : ProcessModbus
 * @How to use          : ModBus main core! Call this function into main!
 */
void ProcessModbus(void)
{
    if (Tx_State != RXTX_IDLE) // If answer is ready, send it!
    {
        if (Tx_DelayCounter >= MODBUS_RESPONSE_DELAY)
        {
            TxRTU();
        }
    }
    RxRTU();               // Call this function every cycle
    if (RxDataAvailable()) // If data is ready enter this!
    {
        switch (Rx_Function) // Which function?
        {
#if MODBUS_READ_INPUT_REGISTERS_ENABLED > 0
        case MODBUS_READ_INPUT_REGISTERS:
        {
            HandleModbusReadInputRegisters();
            break;
        }
#endif
#if MODBUS_READ_HOLDING_REGISTERS_ENABLED > 0
        case MODBUS_READ_HOLDING_REGISTERS:
        {
            HandleModbusReadHoldingRegisters();
            break;
        }
#endif
#if MODBUSWRITE_SINGLE_REGISTER_ENABLED > 0
        case MODBUS_WRITE_SINGLE_REGISTER:
        {
            HandleModbusWriteSingleRegister();
            break;
        }
#endif
#if MODBUS_WRITE_MULTIPLE_REGISTERS_ENABLED > 0
        case MODBUS_WRITE_MULTIPLE_REGISTERS:
        {
            HandleModbusWriteMultipleRegisters();
            break;
        }
#endif
        default:
        {
            HandleModbusError(ERROR_CODE_01);
            break;
        }
        }
    }
}
/******************************************************************************/

/*
 * Function Name        : InitModbus
 * @How to use          : ModBus slave initialize
 */
void InitModbus(unsigned char ModbusSlaveAddress)
{
    MODBUS_SLAVE_ADDRESS = ModbusSlaveAddress;
    ModBus_UART_Initialise();
}
/******************************************************************************/
