/*
 * Created on Thu Oct 22 2026
 *
 * Copyright (c) 2026 by magner mr.jaedong@gmail.com
 */

#include <stdint.h>

#include "modbus_control.h"
#include "Modbus.h"
#include "modbus_registers.h"
#include "settings.h"
#include "soft_timer.h"
#include "meter.h"
#include "eeprom.h"

/* Уровни доступа и пароли (ЗАМЕНИТЬ на постоянные прибора) */
enum
{
    MENUPASS_SYSTEM,
    MENUPASS_METER,
    MENUPASS_SERVICE,
    MENUPASS_COUNT
};
static const uint16_t menu_pass[MENUPASS_COUNT] = {1000, 2000, 3000};

/* Бит ошибки доступа в IREG_STATUS; снимается автоматически через таймаут */
#define FSTATUS_ACCESERROR 0
#define ACCES_ERROR_TIMEOUT_MS 10000u

/* Примечание об атомарности: в отличие от проекта-донора (FreeRTOS, задачи),
 * здесь всё выполняется в главном контексте — главный цикл и 100-мс колбэк
 * не вытесняют друг друга, ISR настройки не читают. Блокировки не нужны. */

/*----------------------------------------------------------------------------*/

static void accessError(uint16_t *status, uint32_t *deadline)
{
    *status |= (uint16_t)(1 << FSTATUS_ACCESERROR);
    *deadline = soft_timer_uptime + ACCES_ERROR_TIMEOUT_MS;
}

/*----------------------------------------------------------------------------*/

/* Выгрузка настроек в holding-регистры (команда HCTRL_ALL_READ).
 * Битовые поля собираются по карте modbus_registers.h */
static void settingsToRegisters(void)
{
    uint16_t rs = 0;
    rs |= (uint16_t)(g_settings.interfaceRS485_baudrate & 0xF); /* биты 3:0 */
    rs |= (uint16_t)((g_settings.interfaceRS485_parity & 0x3) << 4);
    rs |= (uint16_t)((g_settings.interfaceRS485_stopbits & 0x1) << 5);
    rs |= (uint16_t)((g_settings.interfaceRS485_mode & 0x3) << 6);
    rs |= (uint16_t)((g_settings.interfaceRS485_delay & 0xFF) << 8);
    holdingRegisters[HREG_interfaceRS485_configuration] = rs;
    holdingRegisters[HREG_interfaceRS485_slave_address] = g_settings.interfaceRS485_slave_address;

    modbusFloatToRegister(g_settings.aout_calibration_low, &holdingRegisters[HREG_aout_calibration_low]);
    modbusFloatToRegister(g_settings.aout_calibration_high, &holdingRegisters[HREG_aout_calibration_high]);
    modbusLongToRegister(g_settings.serial_number, &holdingRegisters[HREG_serial_number]);

    uint16_t mc = 0;
    mc |= (uint16_t)(g_settings.multiplier_units & 0x3); /* биты 1:0 */
    mc |= (uint16_t)((g_settings.multiplier_parameter & 0x1) << 2);
    mc |= (uint16_t)((g_settings.linearisation & 0x1) << 3);
    holdingRegisters[HREG_multiplier_configuration] = mc;
    modbusFloatToRegister(g_settings.multiplier_value_average, &holdingRegisters[HREG_multiplier_value]);
    modbusFloatToRegister(g_settings.meter_correction, &holdingRegisters[HREG_correction]);

    /* таблицы линеаризации — читаются общим потоком */
    floatToRegCopy(lpoint_forward_frequency, &holdingRegisters[HREG_LPF1], LPOINTS);
    floatToRegCopy(lpoint_forward_value, &holdingRegisters[HREG_LPV1], LPOINTS);

    /* HREG_setpoint_qmax/qmin: полей в SETTINGS_TABLE пока нет — TODO */
}

/*----------------------------------------------------------------------------*/

/* Диспетчер управляющих команд. Вызывать из главного цикла: при пустом
 * HREG_CONTROL и снятой ошибке доступа немедленно возвращается. */
void modbusControl(void)
{
    static uint16_t status = 0;
    static uint32_t acces_deadline = 0;

    uint16_t control = holdingRegisters[HREG_CONTROL];
    if (control == EMPTY && !(status & (1 << FSTATUS_ACCESERROR)))
        return;

    /* автоочистка бита ошибки доступа по таймауту */
    if ((status & (1 << FSTATUS_ACCESERROR)) &&
        (int32_t)(soft_timer_uptime - acces_deadline) >= 0)
    {
        status &= (uint16_t)~(1 << FSTATUS_ACCESERROR);
    }

    if (!(status & (1 << FSTATUS_ACCESERROR)))
    {
        uint16_t password = holdingRegisters[HREG_PASSWORD];

        /* системные параметры (RS-485, калибровка АВ, серийник) */
        if (control == HCTRL_SYSTEM_WRITE)
        {
            if (password == menu_pass[MENUPASS_SYSTEM])
            {
                uint16_t rs = holdingRegisters[HREG_interfaceRS485_configuration];
                settings_set(SET_interfaceRS485_baudrate, (float)(rs & 0xF));
                settings_set(SET_interfaceRS485_parity, (float)((rs >> 4) & 0x3));
                settings_set(SET_interfaceRS485_stopbits, (float)((rs >> 5) & 0x1));
                settings_set(SET_interfaceRS485_mode, (float)((rs >> 6) & 0x3));
                settings_set(SET_interfaceRS485_delay, (float)((rs >> 8) & 0xFF));
                settings_set(SET_interfaceRS485_slave_address, (float)holdingRegisters[HREG_interfaceRS485_slave_address]);
                settings_set(SET_aout_calibration_low, modbusRegisterToFloat(&holdingRegisters[HREG_aout_calibration_low]));
                settings_set(SET_aout_calibration_high, modbusRegisterToFloat(&holdingRegisters[HREG_aout_calibration_high]));
                /* TODO: settings_save_flash(); параметры RS-485 применяются
                 * после перезагрузки (InitModbus читает g_settings) */
                control = HCTRL_ALL_READ;
            }
            else
                accessError(&status, &acces_deadline); /* неверный пароль */
        }
        /* параметры счётчика: множитель, линеаризация, поправка и таблицы
         * линеаризации LPF/LPV — одним потоком с настройками измерителя */
        else if (control == HCTRL_METER_WRITE)
        {
            if (password == menu_pass[MENUPASS_METER])
            {
                settings_set(SET_serial_number, (float)modbusRegisterToLong(&holdingRegisters[HREG_serial_number]));
                uint16_t mc = holdingRegisters[HREG_multiplier_configuration];
                settings_set(SET_multiplier_units, (float)(mc & 0x3));
                settings_set(SET_multiplier_parameter, (float)((mc >> 2) & 0x1));
                settings_set(SET_linearisation, (float)((mc >> 3) & 0x1));
                settings_set(SET_multiplier_value_average, modbusRegisterToFloat(&holdingRegisters[HREG_multiplier_value]));
                settings_set(SET_meter_correction, modbusRegisterToFloat(&holdingRegisters[HREG_correction]));
                regToFloatCopy(&holdingRegisters[HREG_LPF1], lpoint_forward_frequency, LPOINTS);
                regToFloatCopy(&holdingRegisters[HREG_LPV1], lpoint_forward_value, LPOINTS);
                settingsLpoints(); /* пересчитать lpoints_count и нормализовать */
                control = HCTRL_ALL_READ;
            }
            else
                accessError(&status, &acces_deadline);
        }
        /* сервисный сброс: обнуление накопителей объёма и импульсов */
        else if (control == HCTRL_SERVICE_RESET)
        {
            if (password == menu_pass[MENUPASS_SERVICE])
            {
                totalnl = 0;
                gtotalnl = 0;
                impulses = 0;
                critical_payload_t cp = {
                    .totalnl = 0,
                    .gtotalnl = 0,
                    .runtime_device = runtime_device,
                    .runtime_flowmeter = runtime_flowmeter,
                    .impulses = 0,
                    .poweron_counter = poweron_counter};
                critical_save(&cp, false); /* зафиксировать сброс сразу */
            }
            else
                accessError(&status, &acces_deadline);
        }
        /* возврат заводских настроек (накопители НЕ трогаются —
         * для этого HCTRL_SERVICE_RESET) */
        else if (control == HCTRL_DEFAULT)
        {
            if (password == menu_pass[MENUPASS_SYSTEM])
            {
                settings_load_factory(); /* значения из дескрипторов */
                for (uint8_t i = 0; i < LPOINTS; i++) /* таблицы линеаризации */
                {
                    lpoint_forward_frequency[i] = factory_lpoint_forward_frequency[i];
                    lpoint_forward_value[i] = factory_lpoint_forward_value[i];
                }
                settingsLpoints();
                /* ВНИМАНИЕ: заводские параметры RS-485 (адрес/скорость/
                 * чётность) применятся после перезагрузки — текущая
                 * сессия связи продолжает работать со старыми */
                control = HCTRL_ALL_READ;
            }
            else
                accessError(&status, &acces_deadline);
        }
    }

    /* выгрузка в holding-регистры по команде чтения */
    if (control == HCTRL_ALL_READ)
    {
        settingsToRegisters();
    }

    inputRegisters[IREG_STATUS] = status;
    holdingRegisters[HREG_CONTROL] = 0;
}
