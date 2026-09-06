#include "debug.h"
#include "Modbus.h"
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

typedef struct
{
    unsigned char Address;
    unsigned char Function;
    unsigned char DataBuf[MODBUS_RXTX_BUFFER_SIZE];
    unsigned short DataLen;
} RXTX_DATA;

/**********************Slave Transmit and Receive Variables********************/
RXTX_DATA Tx_Data;
unsigned int Tx_Current = 0;
unsigned int Tx_CRC16 = 0xFFFF;
RXTX_STATE Tx_State = RXTX_IDLE;
unsigned char Tx_Buf[MODBUS_TRANSMIT_BUFFER_SIZE];
unsigned int Tx_Buf_Size = 0;

RXTX_DATA Rx_Data;
unsigned int Rx_CRC16 = 0xFFFF;
RXTX_STATE Rx_State = RXTX_IDLE;
unsigned char Rx_Data_Available = FALSE;

volatile unsigned short ModbusTimerValue = 0;

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
    ModBus_UART_String(Tx_Buf, Tx_Buf_Size);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
        ;
    set_rs485_de_disable();
    Tx_Buf_Size = 0;
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

    Tx_Current = 0;
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
    // Initialise the output buffer. The first byte in the buffer says how many registers we have read
    Tx_Data.Function = Rx_Data.Function | 0x80;
    Tx_Data.Address = MODBUS_SLAVE_ADDRESS;
    Tx_Data.DataLen = 1;
    Tx_Data.DataBuf[0] = ErrorCode;
    SendMessage();
}

/******************************************************************************/

/*
 * Function Name        : HandleModbusReadHoldingRegisters
 * @How to use          : Modbus function 03 - Read holding registers
 */
#if MODBUS_READ_HOLDING_REGISTERS_ENABLED > 0
void HandleModbusReadHoldingRegisters(void)
{
    // Holding registers are effectively numerical outputs that can be written to by the host.
    // They can be control registers or analogue outputs.
    // We potientially have one - the pwm output value
    unsigned int StartAddress = 0;
    unsigned int NumberOfRegisters = 0;
    unsigned int i = 0;

    // The message contains the requested start address and number of registers
    StartAddress = ((unsigned int)(Rx_Data.DataBuf[0]) << 8) + (unsigned int)(Rx_Data.DataBuf[1]);
    NumberOfRegisters = ((unsigned int)(Rx_Data.DataBuf[2]) << 8) + (unsigned int)(Rx_Data.DataBuf[3]);

    // If it is bigger than RegisterNumber return error to Modbus Master
    if ((StartAddress + NumberOfRegisters) > NUMBER_OF_OUTPUT_REGISTERS)
        HandleModbusError(ERROR_CODE_02);
    else
    {
        // Initialise the output buffer. The first byte in the buffer says how many registers we have read
        Tx_Data.Function = MODBUS_READ_HOLDING_REGISTERS;
        Tx_Data.Address = MODBUS_SLAVE_ADDRESS;
        Tx_Data.DataLen = 1;
        Tx_Data.DataBuf[0] = 0;

        for (i = 0; i < NumberOfRegisters; i++)
        {
            unsigned short CurrentData = Registers[StartAddress + i].ActValue;

            Tx_Data.DataBuf[Tx_Data.DataLen] = (unsigned char)((CurrentData & 0xFF00) >> 8);
            Tx_Data.DataBuf[Tx_Data.DataLen + 1] = (unsigned char)(CurrentData & 0xFF);
            Tx_Data.DataLen += 2;
            Tx_Data.DataBuf[0] = Tx_Data.DataLen - 1;
        }

        SendMessage();
    }
}
#endif

/******************************************************************************/

/*
 * Function Name        : HandleModbusReadInputRegisters
 * @How to use          : Modbus function 06 - Write single register
 */
#if MODBUSWRITE_SINGLE_REGISTER_ENABLED > 0
void HandleModbusWriteSingleRegister(void)
{
    // Write single numerical output
    unsigned int Address = 0;
    unsigned int Value = 0;
    unsigned char i = 0;

    // The message contains the requested start address and number of registers
    Address = ((unsigned int)(Rx_Data.DataBuf[0]) << 8) + (unsigned int)(Rx_Data.DataBuf[1]);
    Value = ((unsigned int)(Rx_Data.DataBuf[2]) << 8) + (unsigned int)(Rx_Data.DataBuf[3]);

    // Initialise the output buffer. The first byte in the buffer says how many registers we have read
    Tx_Data.Function = MODBUS_WRITE_SINGLE_REGISTER;
    Tx_Data.Address = MODBUS_SLAVE_ADDRESS;
    Tx_Data.DataLen = 4;

    if (Address >= NUMBER_OF_OUTPUT_REGISTERS)
        HandleModbusError(ERROR_CODE_02);
    else
    {
        Registers[Address].ActValue = Value;
        // Output data buffer is exact copy of input buffer
        for (i = 0; i < 4; ++i)
            Tx_Data.DataBuf[i] = Rx_Data.DataBuf[i];
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
    // Write single numerical output
    unsigned int StartAddress = 0;
    // unsigned char ByteCount = 0;
    unsigned int NumberOfRegisters = 0;
    unsigned char i = 0;
    unsigned int Value = 0;

    // The message contains the requested start address and number of registers
    StartAddress = ((unsigned int)(Rx_Data.DataBuf[0]) << 8) + (unsigned int)(Rx_Data.DataBuf[1]);
    NumberOfRegisters = ((unsigned int)(Rx_Data.DataBuf[2]) << 8) + (unsigned int)(Rx_Data.DataBuf[3]);
    // ByteCount = Rx_Data.DataBuf[4];

    // If it is bigger than RegisterNumber return error to Modbus Master
    if ((StartAddress + NumberOfRegisters) > NUMBER_OF_OUTPUT_REGISTERS)
        HandleModbusError(ERROR_CODE_02);
    else
    {
        // Initialise the output buffer. The first byte in the buffer says how many outputs we have set
        Tx_Data.Function = MODBUS_WRITE_MULTIPLE_REGISTERS;
        Tx_Data.Address = MODBUS_SLAVE_ADDRESS;
        Tx_Data.DataLen = 4;
        Tx_Data.DataBuf[0] = Rx_Data.DataBuf[0];
        Tx_Data.DataBuf[1] = Rx_Data.DataBuf[1];
        Tx_Data.DataBuf[2] = Rx_Data.DataBuf[2];
        Tx_Data.DataBuf[3] = Rx_Data.DataBuf[3];

        // Output data buffer is exact copy of input buffer
        for (i = 0; i < NumberOfRegisters; i++)
        {
            Value = (Rx_Data.DataBuf[5 + 2 * i] << 8) + (Rx_Data.DataBuf[6 + 2 * i]);
            Registers[StartAddress + i].ActValue = Value;
        }

        SendMessage();
    }
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
        Rx_Data.Address = ReceiveBuffer[0];
        Rx_CRC16 = 0xffff;
        CRC16(Rx_Data.Address, &Rx_CRC16);
        Rx_Data.Function = ReceiveBuffer[1];
        CRC16(Rx_Data.Function, &Rx_CRC16);

        Rx_Data.DataLen = 0;

        for (i = 2; i < ReceiveCounter; i++)
            Rx_Data.DataBuf[Rx_Data.DataLen++] = ReceiveBuffer[i];

        Rx_State = RXTX_DATABUF;

        ReceiveCounter = 0;
    }

    CheckRxTimeout();

    if ((Rx_State == RXTX_DATABUF) && (Rx_Data.DataLen >= 2))
    {
        // Finish off our CRC check
        Rx_Data.DataLen -= 2;
        for (i = 0; i < Rx_Data.DataLen; ++i)
        {
            CRC16(Rx_Data.DataBuf[i], &Rx_CRC16);
        }

        if (((unsigned int)Rx_Data.DataBuf[Rx_Data.DataLen] + ((unsigned int)Rx_Data.DataBuf[Rx_Data.DataLen + 1] << 8)) == Rx_CRC16)
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
    Tx_Buf_Size = 0;
    Tx_Buf[Tx_Buf_Size++] = Tx_Data.Address;
    CRC16(Tx_Data.Address, &Tx_CRC16);
    Tx_Buf[Tx_Buf_Size++] = Tx_Data.Function;
    CRC16(Tx_Data.Function, &Tx_CRC16);

    for (Tx_Current = 0; Tx_Current < Tx_Data.DataLen; Tx_Current++)
    {
        Tx_Buf[Tx_Buf_Size++] = Tx_Data.DataBuf[Tx_Current];
        CRC16(Tx_Data.DataBuf[Tx_Current], &Tx_CRC16);
    }

    Tx_Buf[Tx_Buf_Size++] = Tx_CRC16 & 0x00FF;
    Tx_Buf[Tx_Buf_Size++] = (Tx_CRC16 & 0xFF00) >> 8;

    if (DoSlaveTX())
    {
        // set_rs485_de_enable();
        // set_rs485_de_disable();
    }
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
        // set_rs485_de_enable();
        // set_rs485_de_disable();

        TxRTU();
    }

    RxRTU(); // Call this function every cycle

    if (RxDataAvailable()) // If data is ready enter this!
    {
        if (Rx_Data.Address == MODBUS_SLAVE_ADDRESS) // Is Data for us?
        {
            switch (Rx_Data.Function) // Data is for us but which function?
            {
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
    ModBus_TIMER_Initialise();
}

/******************************************************************************/
