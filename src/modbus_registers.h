#ifndef _MODBUS_
#define _MODBUS_

#include <stdint.h>

#define PREV_LONG_MEMBER(NAME) NAME##_dummy, NAME = NAME##_dummy - 1 + 2 // enum with step
#define NUMBER_OF_INPUT_REGISTERS IREG_LASTREGISTER
#define NUMBER_OF_HOLDING_REGISTERS HREG_LASTREGISTER
/*----------------------------------------------------------------------------*/

// input registers
enum
{
    IREG_STATUS,                              // format:U, type:bit, unit:бит
    IREG_RATELPM,                             // format:F, type:f32, unit:л/м
    PREV_LONG_MEMBER(IREG_RATE),              // format:F, type:f32, unit:тек.
    PREV_LONG_MEMBER(IREG_TOTAL),             // format:F, type:f32, unit:тек.
    PREV_LONG_MEMBER(IREG_GTOTAL),            // format:F, type:f32, unit:тек.
    PREV_LONG_MEMBER(IREG_IRATEML),           // format:UL, type:u32, unit:мл/м
    PREV_LONG_MEMBER(IREG_ITOTALML),          // format:UL, type:u32, unit:мл
    PREV_LONG_MEMBER(IREG_IGTOTALML),         // format:UL, type:u32, unit:мл
    PREV_LONG_MEMBER(IREG_FREQUENCY),         // format:F, type:f32, unit:Гц
    PREV_LONG_MEMBER(IREG_IMPULSES),          // format:UL, type:u32, unit:
    PREV_LONG_MEMBER(IREG_RATEMAXLPM),        // format:F, type:f32, unit:л/м
    PREV_LONG_MEMBER(IREG_RUNTIME_FLOWMETER), // format:UL, type:u32, unit:с
    PREV_LONG_MEMBER(IREG_RUNTIME_DEVICE),    // format:UL, type:u32, unit:с
    PREV_LONG_MEMBER(IREG_POWERON_NUMBER),    // format:U, type:U16, unit:
    IREG_METERCRC16,                          // format:U, type:U16, unit:
    IREG_ESTATUS,                             // format:U, type:binary, unit:бит
    IREG_DEBUG1,                              // format:U, type:binary, unit:бит
    IREG_DEBUG2,                              // format:U, type:U16, unit:
    IREG_DEBUG3,                              // format:U, type:U16, unit:
    IREG_DEBUG4,                              // format:U, type:U16, unit:
    IREG_DEBUG5,                              // format:U, type:U16, unit:
    IREG_DEBUG6,                              // format:U, type:U16, unit:
    IREG_DEBUG7,                              // format:U, type:U16, unit:
    IREG_DEBUG8,                              // format:U, type:U16, unit:
    IREG_LASTREGISTER                         // last input register mark
};

// format: U - unsigned, F - float, UL - unsigned long
// type: f32 - float, b - bool, e - enum, i8 - integer 8bit, u8 - unsigned 8bit, u16 - unsigned 16bit, u32 - unsigned 32bit, bit - bitwise value

// holding registers
enum
{
    HREG_CONTROL,                                    // format:U, type:u16
    HREG_PASSWORD,                                   // format:U, type:u16
    HREG_interfaceRS485_configuration,               // format:U, type:bit /* RS-485 configuration bits: delay - 15:8, function - 7:6 ,stopbits - 5, parity - 4, baud - 3:0 (0 - 1200, 1 - 2400, 2 - 4800, 3 - 9600, 4 - 14400, 5 - 19200, 6 - 28800, 7 - 38400, 8 - 57600, 9 - 115200) */
    HREG_interfaceRS485_slave_address,               // format:U, type:u8
    HREG_aout_calibration_low,                       // format:F, type:f32
    PREV_LONG_MEMBER(HREG_aout_calibration_high),    // format:F, type:f32
    PREV_LONG_MEMBER(HREG_setpoint_qmax),            // format:F, type:f32
    PREV_LONG_MEMBER(HREG_setpoint_qmin),            // format:F, type:f32
    PREV_LONG_MEMBER(HREG_serial_number),            // format:UL, type:u32
    PREV_LONG_MEMBER(HREG_multiplier_configuration), // format:U, type:bit /* Multiplier configuration bits: linearistaion - 3, parametr - 2, units - 1:0 */
    HREG_multiplier_value,                           // format:F, type:f32
    PREV_LONG_MEMBER(HREG_correction),               // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPF1),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPF2),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPF3),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPF4),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPF5),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPF6),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPF7),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPF8),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPF9),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPF10),                    // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPV1),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPV2),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPV3),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPV4),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPV5),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPV6),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPV7),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPV8),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPV9),                     // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LPV10),                    // format:F, type:f32
    PREV_LONG_MEMBER(HREG_LASTREGISTER)              // last holding register mark
};

enum
{
    EMPTY,
    HCTRL_SYSTEM_WRITE,     // password SYSTEM
    HCTRL_METER_WRITE,      // password METER
    HCTRL_METER_LPFV_WRITE, // linearisation points forward value set - password METER
    HCTRL_METER_LPFF_WRITE, // linearisation points forward frequenvy set - password METER
    HCTRL_SERVICE_RESET,    // password SERVICE
    HCTRL_METER_LPFV_READ,  // linearisation points forward value read
    HCTRL_METER_LPFF_READ,  // linearisation points forward frequency read
    HCTRL_ALL_READ,         // read back
};
/*----------------------------------------------------------------------------*/

void modbusLongToRegister(uint32_t src, uint16_t *dst);
uint32_t modbusRegisterToLong(uint16_t *src);
void modbusFloatToRegister(float src, uint16_t *dst);
float modbusRegisterToFloat(uint16_t *src);

void regToFloatCopy(uint16_t dest[], float source[], uint8_t count);
void floatToRegCopy(float source[], uint16_t dest[], uint8_t count);

#endif // !_MODBUS_