/*
 * Created on Mon Jan 27 2025
 *
 * Copyright (c) 2025 by magner mr.jaedong@gmail.com
 */

#include "modbus_registers.h"
#include <stdint.h>
#include <string.h>
/*----------------------------------------------------------------------------*/

// Запись uint32_t в два регистра
void modbusLongToRegister(uint32_t src, uint16_t *dst)
{
    dst[0] = (src >> 16) & 0xFFFF; // Старшее слово
    dst[1] = src & 0xFFFF;         // Младшее слово
}

/*----------------------------------------------------------------------------*/
// Чтение uint32_t из двух регистров
uint32_t modbusRegisterToLong(uint16_t *src)
{
    return ((uint32_t)src[0] << 16) | (src[1] & 0xFFFF);
}

/*----------------------------------------------------------------------------*/
// Запись float в два регистра (универсально)
void modbusFloatToRegister(float src, uint16_t *dst)
{
    uint32_t temp;
    memcpy(&temp, &src, sizeof(float));
    dst[1] = (temp >> 16) & 0xFFFF;
    dst[0] = temp & 0xFFFF;
}

/*----------------------------------------------------------------------------*/
// Чтение float из двух регистров (универсально)
float modbusRegisterToFloat(uint16_t *src)
{
    uint32_t temp;
    float result;
    temp = ((uint32_t)src[0] << 16) | (src[1] & 0xFFFF);
    memcpy(&result, &temp, sizeof(float));
    return result;
}

/*----------------------------------------------------------------------------*/
// Массовое копирование массива float в регистры
void floatToRegCopy(float source[], uint16_t dest[], uint8_t count)
{
    uint32_t temp;
    for (uint8_t i = 0; i < count; i++)
    {
        memcpy(&temp, &source[i], sizeof(float));
        dest[i * 2] = (temp >> 16) & 0xFFFF;
        dest[i * 2 + 1] = temp & 0xFFFF;
    }
}

/*----------------------------------------------------------------------------*/
// Массовое копирование регистров в массив float
void regToFloatCopy(uint16_t dest[], float source[], uint8_t count)
{
    uint32_t temp;
    for (uint8_t i = 0; i < count; i++)
    {
        temp = ((uint32_t)dest[i * 2] << 16) | (dest[i * 2 + 1] & 0xFFFF);
        memcpy(&source[i], &temp, sizeof(float));
    }
}