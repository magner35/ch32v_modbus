/*
 * Modbus Version 1.0
 */
#include "ModbusPort.h"

#ifndef __MODBUS__H
#define __MODBUS__H

#define NUMBER_OF_OUTPUT_REGISTERS 10 // Modbus RTU Slave Output Register Number
                                      // Have to put a number of registers here
                                      // It has to be bigger than 0 (zero)!!
#define MODBUS_TIMEOUTTIMER 250       // Timeout Constant for Modbus RTU Slave [millisecond]

#define MODBUS_READ_HOLDING_REGISTERS_ENABLED (1)   // If you want to use make it 1, or 0
#define MODBUSWRITE_SINGLE_REGISTER_ENABLED (1)     // If you want to use make it 1, or 0
#define MODBUS_WRITE_MULTIPLE_REGISTERS_ENABLED (1) // If you want to use make it 1, or 0

/****************************Don't Touch This**********************************/
// Buffers for Modbus RTU Slave
#define MODBUS_RECEIVE_BUFFER_SIZE (NUMBER_OF_OUTPUT_REGISTERS * 2 + 5)
#define MODBUS_TRANSMIT_BUFFER_SIZE MODBUS_RECEIVE_BUFFER_SIZE
#define MODBUS_RXTX_BUFFER_SIZE MODBUS_TRANSMIT_BUFFER_SIZE

// Variable for Slave Address
extern unsigned char MODBUS_SLAVE_ADDRESS; // Modbus RTU Slave address number [0 to 255]

typedef struct
{
    short ActValue;
} RegStructure;

extern RegStructure holdingRegisters[NUMBER_OF_OUTPUT_REGISTERS];
extern volatile unsigned short ModbusTimerValue;

extern volatile unsigned char Tx_Buf[];
extern volatile unsigned int Tx_Buf_Size;
extern volatile unsigned int Tx_Index;

// Main Functions
extern void InitModbus(unsigned char ModbusSlaveAddress);
extern void ProcessModbus(void);

#endif
