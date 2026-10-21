#ifndef __MODBUS__H
#define __MODBUS__H

#include <stdint.h>
#include "modbus_registers.h"

#define MODBUS_RESPONSE_DELAY 5
#define MODBUS_TIMEOUTTIMER 250 // Timeout Constant for Modbus RTU Slave [millisecond]

#define MODBUS_READ_HOLDING_REGISTERS_ENABLED (1)   // If you want to use make it 1, or 0
#define MODBUS_READ_INPUT_REGISTERS_ENABLED (1)     // If you want to use make it 1, or 0
#define MODBUSWRITE_SINGLE_REGISTER_ENABLED (1)     // If you want to use make it 1, or 0
#define MODBUS_WRITE_MULTIPLE_REGISTERS_ENABLED (1) // If you want to use make it 1, or 0

/****************************Don't Touch This**********************************/
/* Буферы фиксированы и НЕ зависят от числа регистров. 160 Б покрывает
 * максимальный ЛЕГИТИМНЫЙ кадр для карты до ~77 регистров (запись всех
 * holding разом: 66 рег. → 141 Б). Кадры длиннее буфера отбрасываются
 * по CRC. Поднять до 256 (полный лимит RTU), если позволит RAM.
 * Приёмный буфер один — ReceiveBuffer (ModbusPort), запрос разбирается
 * из него на месте; ответ строится в Tx_Buf без промежуточных копий. */
#define MODBUS_RECEIVE_BUFFER_SIZE 160
#define MODBUS_TRANSMIT_BUFFER_SIZE 160

// Variable for Slave Address
extern unsigned char MODBUS_SLAVE_ADDRESS; // Modbus RTU Slave address number [0 to 255]

/* Регистровые карты — плоские массивы uint16 (один элемент = один регистр).
 * Многорегистровые значения (float/U32) записываются через хелперы
 * modbus_registers.h по индексу первого регистра значения */
extern uint16_t holdingRegisters[NUMBER_OF_HOLDING_REGISTERS];
extern uint16_t inputRegisters[NUMBER_OF_INPUT_REGISTERS];
extern volatile unsigned short ModbusTimerValue;
extern volatile unsigned char Tx_Buf[];
extern volatile uint8_t Tx_Buf_Size;
extern volatile uint8_t Tx_Index;
extern volatile uint8_t Tx_DelayCounter;

// Main Functions
extern void InitModbus(unsigned char ModbusSlaveAddress);
extern void ProcessModbus(void);

#endif
