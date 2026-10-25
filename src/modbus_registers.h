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
    IREG_STATUS,                              // format:U, type:bit, unit:бит (enum FSTATUS)
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
    IREG_DEBUG1,                              // format:U, type:binary, unit:бит
    IREG_DEBUG2,                              // format:U, type:U16, unit:
    IREG_DEBUG3,                              // format:U, type:U16, unit:
    IREG_DEBUG4,                              // format:U, type:U16, unit:
    IREG_DEBUG5,                              // format:U, type:U16, unit:
    IREG_LASTREGISTER                         // last input register mark
};

// format: U - unsigned, F - float, UL - unsigned long
// type: f32 - float, b - bool, e - enum, i8 - integer 8bit, u8 - unsigned 8bit, u16 - unsigned 16bit, u32 - unsigned 32bit, bit - bitwise value

// IREG_STATUS bits
enum
{
    FSTATUS_ACCESERROR, /* ошибка доступа, снимается автоматически через таймаут */
    FSTATUS_FLOW,       /*  */
    FSTATUS_RATEHIGH,   /*  */
    FSTATUS_RATELOW,    /*  */
    FSTATUS_HIGHFREQ,   /*  */
    FSTATUS_LOWFREQ,    /*  */
    FSTATUS_DUTY,       /*  */
    FSTATUS_FAULT       /*  */
};

// holding registers
enum
{
    HREG_CONTROL,                                 // type:u16
    HREG_PASSWORD,                                // type:u16
    HREG_serial_number,                           // type:u32
    PREV_LONG_MEMBER(HREG_units_time_base),       // type:u16
    HREG_units_rate,                              // type:u16
    HREG_units_total,                             // type:u16
    HREG_aout_parameter,                          // type:u16
    HREG_aout_treshold_4ma,                       // type:f32
    PREV_LONG_MEMBER(HREG_aout_treshold_20ma),    // type:f32
    PREV_LONG_MEMBER(HREG_aout_calibration_low),  // type:f32
    PREV_LONG_MEMBER(HREG_aout_calibration_high), // type:f32
    PREV_LONG_MEMBER(HREG_rate_damping),          // type:u16
    HREG_rate_averaging,                          // type:u16
    HREG_input_sens,                              // type:u16
    HREG_sensor_filter_freq_high,                 // type:u16
    HREG_sensor_filter_freq_low,                  // type:u16
    HREG_sensor_filter_duty,                      // type:u16
    HREG_interfaceRS485_slave_address,            // type:u16
    HREG_interfaceRS485_baudrate,                 // type:u16
    HREG_interfaceRS485_parity,                   // type:u16
    HREG_interfaceRS485_stopbits,                 // type:u16
    HREG_interfaceRS485_delay,                    // type:u16
    HREG_interfaceRS485_mode,                     // type:u16
    HREG_setpoint_qmax,                           // type:f32
    PREV_LONG_MEMBER(HREG_setpoint_qmin),         // type:f32
    PREV_LONG_MEMBER(HREG_scaled_pulse),          // type:f32
    PREV_LONG_MEMBER(HREG_multiplier_parameter),  // type:u16
    HREG_multiplier_units,                        // type:u16
    HREG_meter_correction,                        // type:f32
    PREV_LONG_MEMBER(HREG_LPF1),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPF2),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPF3),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPF4),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPF5),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPF6),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPF7),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPF8),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPF9),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPF10),                 // type:f32
    PREV_LONG_MEMBER(HREG_LPV1),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPV2),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPV3),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPV4),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPV5),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPV6),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPV7),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPV8),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPV9),                  // type:f32
    PREV_LONG_MEMBER(HREG_LPV10),                 // type:f32
    PREV_LONG_MEMBER(HREG_LASTREGISTER)           // last holding register mark
};

enum
{
    EMPTY,
    HCTRL_SYSTEM_WRITE,  // password SYSTEM
    HCTRL_METER_WRITE,   // password METER
    HCTRL_TOTAL_RESET,   // password SYSTEM
    HCTRL_GTOTAL_RESET,  // password SYSTEM
    HCTRL_SERVICE_RESET, // password SERVICE
    HCTRL_ALL_READ,      // read back
    HCTRL_DEFAULT,       // password SERIVCE - возврат заводских настроек
};
/*----------------------------------------------------------------------------*/

void modbusLongToRegister(uint32_t src, uint16_t *dst);
uint32_t modbusRegisterToLong(uint16_t *src);
void modbusFloatToRegister(float src, uint16_t *dst);
float modbusRegisterToFloat(uint16_t *src);

void regToFloatCopy(uint16_t dest[], float source[], uint8_t count);
void floatToRegCopy(float source[], uint16_t dest[], uint8_t count);

#endif // !_MODBUS_