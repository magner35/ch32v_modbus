#ifndef _MODBUS_
#define _MODBUS_

#include <stdint.h>

#define PREV_LONG_MEMBER(NAME) NAME##_dummy, NAME = NAME##_dummy - 1 + 2 // enum with step
#define NUMBER_OF_INPUT_REGISTERS IREG_LASTREGISTER
#define NUMBER_OF_HOLDING_REGISTERS HREG_LASTREGISTER
/*----------------------------------------------------------------------------*/

/*
 * Аннотации в комментариях — источник данных для профиля конфигуратора
 * (tools/gen_profile.py, репозиторий configurator). Формат:
 *   format: U|F|UL|U64, type: FLOAT|U16|U32|U64|..., values: диапазон или
 *   список "0 - X, 1 - Y", label: подпись, tooltip: подсказка,
 *   group: секция интерфейса, pass: уровень пароля, units: единица
 *   (rate/total/gtotal — вычисляемые; прочее — литеральная, например Гц).
 */

// input registers
enum
{
    IREG_STATUS,                              // format:U, type:U16, label:Статус, group:led
    IREG_RATELPM,                             // format:F, type:FLOAT, label:Расход, units:л/мин
    PREV_LONG_MEMBER(IREG_RATE),              // format:F, type:FLOAT, label:Расход, units:rate
    PREV_LONG_MEMBER(IREG_TOTAL),             // format:F, type:FLOAT, label:Объём, tooltip:Объём разностный, units:total
    PREV_LONG_MEMBER(IREG_GTOTAL),            // format:F, type:FLOAT, label:Общий, tooltip:Накопленный объём, units:gtotal
    PREV_LONG_MEMBER(IREG_IRATEML),           // format:UL, type:U32, label:Расход, tooltip:Целочисленный расход, мл/мин, units:мл/мин
    PREV_LONG_MEMBER(IREG_ITOTALML),          // format:UL, type:U32, label:Объём, tooltip:Целочисленный объём, units:мл
    PREV_LONG_MEMBER(IREG_IGTOTALML),         // format:UL, type:U32, label:Общий, tooltip:Целочисленный накопленный объём, units:мл
    PREV_LONG_MEMBER(IREG_FREQUENCY),         // format:F, type:FLOAT, label:Частота, tooltip:Частота импульсов, units:Гц
    PREV_LONG_MEMBER(IREG_IMPULSES),          // format:U64, type:U64, label:Импульсы, tooltip:Количество импульсов (4 регистра, слова ABCD)
    PREV_LONG_MEMBER(IREG_IMPULSES_DUMMY),    // dummy: хвост u64, не регистр
    PREV_LONG_MEMBER(IREG_RATEMAXLPM),        // format:F, type:FLOAT, label:Qmax, tooltip:Максимальный достигнутый расход, units:л/мин
    PREV_LONG_MEMBER(IREG_RUNTIME_FLOWMETER), // format:UL, type:U32, label:НаработкаП, tooltip:Наработка преобразователя, units:с
    PREV_LONG_MEMBER(IREG_RUNTIME_DEVICE),    // format:UL, type:U32, label:НаработкаУ, tooltip:Наработка устройства, units:с
    PREV_LONG_MEMBER(IREG_POWERON_NUMBER),    // format:U, type:U16, label:Включений, tooltip:Количество включений
    IREG_METERCRC16,                          // format:U, type:U16, label:Изм.CRC, tooltip:Контрольная сумма параметров измерителя
    IREG_DEBUG1,                              // format:U, type:U16, label:DEBUG1, group:hidden
    IREG_DEBUG2,                              // format:U, type:U16, label:DEBUG2, group:hidden
    IREG_DEBUG3,                              // format:U, type:U16, label:DEBUG3, group:hidden
    IREG_DEBUG4,                              // format:U, type:U16, label:DEBUG4, group:hidden
    IREG_DEBUG5,                              // format:U, type:U16, label:DEBUG5, group:hidden
    IREG_LASTREGISTER                         // last
};

// IREG_STATUS bits
enum
{
    FSTATUS_ACCESERROR, // label:Ошибка доступа
    FSTATUS_FLOW,       // label:Поток
    FSTATUS_RATEHIGH,   // label:Уставка - Расход+
    FSTATUS_RATELOW,    // label:Уставка - Расход-
    FSTATUS_HIGHFREQ,   // label:Верх.частота
    FSTATUS_LOWFREQ,    // label:Ниж.частота
    FSTATUS_DUTY,       // label:Скважность
    FSTATUS_FAULT       // label:Неисправность
};

// holding registers
enum
{
    HREG_CONTROL,                                 // format:U, type:U16, label:Управление, group:system
    HREG_PASSWORD,                                // format:UL, type:U32, label:Пароль, group:system
    PREV_LONG_MEMBER(HREG_serial_number),         // format:UL, type:U32, values:000000 … 999999, label:Серийный N, tooltip:Серийный номер устройства, group:meter, pass:METER
    PREV_LONG_MEMBER(HREG_units_time_base),       // format:U, type:ENUM, values:0 - Секунда, 1 - Минута, 2 - Час, label:База времени, tooltip:База времени расхода, group:units, pass:SYSTEM
    HREG_units_rate,                              // format:U, type:ENUM, values:0 - Миллилитр, 1 - Литр, 2 - Метр кубический, label:Ед. расхода, group:units, pass:SYSTEM
    HREG_units_total,                             // format:U, type:ENUM, values:0 - Миллилитр, 1 - Литр, 2 - Метр кубический, label:Ед. объёма, group:units, pass:SYSTEM
    HREG_aout_parameter,                          // format:U, type:ENUM, values:0 - Расход, 1 - Объём, 2 - Общий объём, label:AO параметр, tooltip:Переменная аналогового выхода, group:aout, pass:SYSTEM
    HREG_aout_treshold_4ma,                       // format:F, type:FLOAT, label:AO порог 4мА, tooltip:Нижний порог аналогового выхода, group:aout, pass:SYSTEM
    PREV_LONG_MEMBER(HREG_aout_treshold_20ma),    // format:F, type:FLOAT, label:AO порог 20мА, tooltip:Верхний порог аналогового выхода, group:aout, pass:SYSTEM
    PREV_LONG_MEMBER(HREG_aout_calibration_low),  // format:F, type:FLOAT, label:AO калибровка мин, tooltip:Нижнее калибровочное значение АВ, group:aout, pass:SERVICE
    PREV_LONG_MEMBER(HREG_aout_calibration_high), // format:F, type:FLOAT, label:AO калибровка макс, tooltip:Верхнее калибровочное значение АВ, group:aout, pass:SERVICE
    PREV_LONG_MEMBER(HREG_rate_damping),          // format:U, type:U8, values:0 … 99, label:Демпфирование, tooltip:Постоянная времени демпфирования расхода, с, group:sensor, pass:SYSTEM
    HREG_rate_averaging,                          // format:U, type:U8, values:1 … 255, label:Усреднение, tooltip:Количество импульсов для усреднения, group:sensor, pass:SYSTEM
    HREG_input_sens,                              // format:U, type:ENUM, values:0 - 40, 1 - 20, 2 - 10, 3 - 5, label:Чувствительность, tooltip:Чувствительность по входу, мВ, group:sensor, pass:SYSTEM
    HREG_sensor_filter_freq_high,                 // format:U, type:U16, values:0 … 9999, label:ФВЧ, tooltip:Частота среза ФВЧ, units:Гц, group:sensor, pass:SYSTEM
    HREG_sensor_filter_freq_low,                  // format:U, type:U16, values:0 … 9999, label:ФНЧ, tooltip:Частота среза ФНЧ, units:Гц, group:sensor, pass:SYSTEM
    HREG_sensor_filter_duty,                      // format:U, type:U8, values:0 … 100, label:Скважность, tooltip:Q коэф.заполнения, %, group:sensor, pass:SYSTEM
    HREG_interfaceRS485_slave_address,            // format:U, type:U8, values:1 … 247, label:Адрес, group:interface, pass:SYSTEM
    HREG_interfaceRS485_baudrate,                 // format:U, type:ENUM, values:0 - 1200, 1 - 2400, 2 - 4800, 3 - 9600, 4 - 14400, 5 - 19200, 6 - 28800, 7 - 38400, 8 - 57600, 9 - 115200, label:Скорость, group:interface, pass:SYSTEM
    HREG_interfaceRS485_parity,                   // format:U, type:ENUM, values:0 - None, 1 - Even, 2 - Odd, label:Чётность, group:interface, pass:SYSTEM
    HREG_interfaceRS485_stopbits,                 // format:U, type:BOOL, values:0 - 1, 1 - 2, label:Стоп-биты, group:interface, pass:SYSTEM
    HREG_interfaceRS485_delay,                    // format:U, type:U8, values:0 … 255, label:Задержка ответа, tooltip:Задержка ответа, мс, group:interface, pass:SYSTEM
    HREG_interfaceRS485_mode,                     // format:U, type:ENUM, values:0 - RS-485, 1 - Норм. импульс, 2 - Расход вне диапазона, label:Режим выхода, group:interface, pass:SYSTEM
    HREG_setpoint_qmax,                           // format:F, type:FLOAT, label:Qmax, tooltip:Уставка максимального расхода, units:л/м, group:setpoint, pass:SYSTEM
    PREV_LONG_MEMBER(HREG_setpoint_qmin),         // format:F, type:FLOAT, label:Qmin, tooltip:Уставка минимального расхода, units:л/м, group:setpoint, pass:SYSTEM
    PREV_LONG_MEMBER(HREG_scaled_pulse),          // format:F, type:FLOAT, label:Цена импульса, tooltip:Цена нормируемого импульса, group:setpoint, pass:SYSTEM
    PREV_LONG_MEMBER(HREG_multiplier_parameter),  // format:U, type:BOOL, values:0 - K-фактор, 1 - Цена импульса, label:Тип, tooltip:Тип множителя расхода, group:meter, pass:METER
    HREG_multiplier_units,                        // format:U, type:ENUM, values:0 - Миллилитр, 1 - Литр, 2 - Метр кубический, label:Единицы, tooltip:Единицы измерения множителя расхода, group:meter, pass:METER
    HREG_meter_correction,                        // format:F, type:FLOAT, label:Поправка, tooltip:Поправочный коэффициент множителя, group:meter, pass:METER
    PREV_LONG_MEMBER(HREG_LPF1),                  // format:F, type:FLOAT, label:Частота1, tooltip:Частота точки линеаризации 1, units:Гц, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPF2),                  // format:F, type:FLOAT, label:Частота2, units:Гц, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPF3),                  // format:F, type:FLOAT, label:Частота3, units:Гц, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPF4),                  // format:F, type:FLOAT, label:Частота4, units:Гц, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPF5),                  // format:F, type:FLOAT, label:Частота5, units:Гц, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPF6),                  // format:F, type:FLOAT, label:Частота6, units:Гц, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPF7),                  // format:F, type:FLOAT, label:Частота7, units:Гц, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPF8),                  // format:F, type:FLOAT, label:Частота8, units:Гц, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPF9),                  // format:F, type:FLOAT, label:Частота9, units:Гц, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPF10),                 // format:F, type:FLOAT, label:Частота10, units:Гц, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPV1),                  // format:F, type:FLOAT, label:Значение1, tooltip:Значение точки линеаризации 1, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPV2),                  // format:F, type:FLOAT, label:Значение2, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPV3),                  // format:F, type:FLOAT, label:Значение3, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPV4),                  // format:F, type:FLOAT, label:Значение4, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPV5),                  // format:F, type:FLOAT, label:Значение5, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPV6),                  // format:F, type:FLOAT, label:Значение6, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPV7),                  // format:F, type:FLOAT, label:Значение7, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPV8),                  // format:F, type:FLOAT, label:Значение8, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPV9),                  // format:F, type:FLOAT, label:Значение9, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LPV10),                 // format:F, type:FLOAT, label:Значение10, group:linearisation, pass:METER
    PREV_LONG_MEMBER(HREG_LASTREGISTER)           // last
};

// Commands
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
void modbusLongLongToRegister(uint64_t src, uint16_t *dst); /* u64: 4 регистра, ABCD */
uint32_t modbusRegisterToLong(uint16_t *src);
void modbusFloatToRegister(float src, uint16_t *dst);
float modbusRegisterToFloat(uint16_t *src);

void regToFloatCopy(uint16_t dest[], float source[], uint8_t count);
void floatToRegCopy(float source[], uint16_t dest[], uint8_t count);

#endif // !_MODBUS_
