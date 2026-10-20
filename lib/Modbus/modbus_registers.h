#ifndef _MODBUS_
#define _MODBUS_

#include <stdint.h>

#define PREV_LONG_MEMBER(NAME) NAME##_dummy, NAME = NAME##_dummy - 1 + 2 // enum with step
#define NUMBER_OF_INPUT_REGISTERS IREG_LASTREGISTER
#define NUMBER_OF_HOLDING_REGISTERS HREG_LASTREGISTER
/*----------------------------------------------------------------------------*/

enum
{
    IREG_STATUS,                              // format:U, type:U16
    IREG_RATE,                                // format:F, type:FLOAT
    PREV_LONG_MEMBER(IREG_RATELPM),           // format:F, type:FLOAT
    PREV_LONG_MEMBER(IREG_TOTAL),             // format:F, type:FLOAT
    PREV_LONG_MEMBER(IREG_GTOTAL),            // format:F, type:FLOAT
    PREV_LONG_MEMBER(IREG_FREQUENCY),         // format:F, type:FLOAT
    PREV_LONG_MEMBER(IREG_IMPULSES),          // format:UL, type:U32
    PREV_LONG_MEMBER(IREG_RATEMAXLPM),        // format:F, type:FLOAT
    PREV_LONG_MEMBER(IREG_RUNTIME_FLOWMETER), // format:UL, type:TIME
    PREV_LONG_MEMBER(IREG_RUNTIME_DEVICE),    // format:UL, type:TIME
    PREV_LONG_MEMBER(IREG_POWERON_NUMBER),    // format:U, type:U16
    IREG_METERCRC16,                          // format:U, type:U16
    IREG_METERTIME,                           // format:UL, type:TIME
    IREG_ESTATUS,                             // format:U, type:U16
    IREG_LASTREGISTER                         // last input register mark
};

// format: U - unsigned, F - float, UL - unsigned long
// type: FLOAT - float, BOOL - bool, ENUM - enum, TIME - UNIX time, I8 - integer 8bit, U8 - unsigned 8bit, U16 - unsigned 16bit, U32 - unsigned 32bit
// values: range of values depending on the type
enum
{
    HREG_CONTROL,                                        // format:U, type:U16, values:bitwise
    HREG_PASSWORD,                                       // format:UL, type:U32, values:0 … 999999
    PREV_LONG_MEMBER(HREG_interfaceRS485_slave_address), // format:U, type:U8, values:0 … 247
    HREG_interfaceRS485_baudrate,                        // format:U, type:ENUM, values:0 - 1200, 1 - 2400, 2 - 4800, 3 - 9600, 4 - 14400, 5 - 19200, 6 - 28800, 7 - 38400, 8 - 57600, 9 - 115200
    HREG_interfaceRS485_parity,                          // format:U, type:ENUM, values:0 - None, 1 - Even, 2 - Odd
    HREG_interfaceRS485_stopbits,                        // format:U, type:BOOL, values:0 - 1, 1 - 2
    HREG_interfaceRS485_delay,                           // format:U, type:U8, values:0 … 255
    HREG_multiplier_parameter,                           // format:U, type:BOOL, values:0 - K-фaктop, 1 - Цeнa импульса
    HREG_multiplier_units,                               // format:U, type:ENUM, values: 0 - Mиллилитp, 1 - Литp, 2 - Meтp кyбический
    HREG_multiplier_value_average,                       // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_linearisation_points),         // format:U, type:U8, values:2 … 10
    HREG_linearisation_method,                           // format:U, type:ENUM, values:0 - Bыключено, 1 - Линeйный, 2 - Cплaйн-A, 3 - Cплaйн-Б
    HREG_linearisation_predict,                          // format:U, type:BOOL, values:0 - Bыключено, 1 - Bключено
    HREG_meter_mode,                                     // format:U, type:ENUM, values:0 - Пpямoй, 1 - Peвepc-A, 2 - Peвepc-Б
    HREG_meter_correction,                               // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_meter_qmax_lpm),               // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_serial_number),                // format:UL, type:U32, values:000000 … 999999
    PREV_LONG_MEMBER(HREG_LPF1),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPF2),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPF3),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPF4),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPF5),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPF6),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPF7),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPF8),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPF9),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPF10),                        // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPV1),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPV2),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPV3),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPV4),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPV5),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPV6),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPV7),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPV8),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPV9),                         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LPV10),                        // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_aout_calibration_low),         // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_aout_calibration_high),        // format:F, type:FLOAT, values:0 … 999999.9
    PREV_LONG_MEMBER(HREG_LASTREGISTER)                  // last holding register mark
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