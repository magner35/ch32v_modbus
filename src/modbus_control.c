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
static const uint32_t menu_pass[MENUPASS_COUNT] = {100000, 200000, 300000};

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
/* Перенос настроек ↔ holding-регистры циклом по дескрипторам:
 * соответствие «настройка ↔ регистр ↔ группа пароля» живёт только
 * в SETTINGS_TABLE (колонки HREG_/SETGRP_), ручной синхронизации нет */
static void settingsToRegisters(void)
{
    for (uint8_t i = 0; i < SETTINGS_COUNT; i++)
    {
        const setting_desc_t *d = &g_settings_desc[i];
        uint16_t *reg = &holdingRegisters[d->hreg];
        void *field = (uint8_t *)&g_settings + g_settings_offs[i];
        switch ((setting_type_t)d->type)
        {
        case ST_FLOAT:
            modbusFloatToRegister(*(float *)field, reg);
            break;
        case ST_U32:
            modbusLongToRegister(*(uint32_t *)field, reg);
            break;
        case ST_U16:
            *reg = *(uint16_t *)field;
            break;
        default: /* BOOL/ENUM/U8/I8 — однобайтовые */
            *reg = *(uint8_t *)field;
            break;
        }
    }
    /* таблицы линеаризации — тем же потоком чтения */
    floatToRegCopy(lpoint_forward_frequency, &holdingRegisters[HREG_LPF1], LPOINTS);
    floatToRegCopy(lpoint_forward_value, &holdingRegisters[HREG_LPV1], LPOINTS);
}

/* Holding → настройки для группы group (валидация в settings_set).
 * Возвращает false, если хоть одно значение отброшено по диапазону */
static bool registersToSettings(uint8_t group)
{
    bool ok = true;
    for (uint8_t i = 0; i < SETTINGS_COUNT; i++)
    {
        const setting_desc_t *d = &g_settings_desc[i];
        if (d->group != group)
            continue;
        uint16_t *reg = &holdingRegisters[d->hreg];
        float val;
        switch ((setting_type_t)d->type)
        {
        case ST_FLOAT:
            val = modbusRegisterToFloat(reg);
            break;
        case ST_U32:
            val = (float)modbusRegisterToLong(reg);
            break;
        default:
            val = *reg;
            break;
        }
        if (!settings_set(i, val))
            ok = false;
    }
    return ok;
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
        uint32_t password = modbusRegisterToLong(&holdingRegisters[HREG_PASSWORD]); /* 2 регистра, ABCD */

        /* системные параметры (RS-485, калибровка АВ, серийник) */
        if (control == HCTRL_SYSTEM_WRITE)
        {
            if (password == menu_pass[MENUPASS_SYSTEM])
            {
                registersToSettings(SETGRP_SYSTEM); /* RS-485 + калибровка АВ */
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
                registersToSettings(SETGRP_METER);
                regToFloatCopy(&holdingRegisters[HREG_LPF1], lpoint_forward_frequency, LPOINTS);
                regToFloatCopy(&holdingRegisters[HREG_LPV1], lpoint_forward_value, LPOINTS);
                settingsLpoints(); /* пересчитать lpoints_count и нормализовать */
                control = HCTRL_ALL_READ;
            }
            else
                accessError(&status, &acces_deadline);
        }
        /* сброс рабочего счётчика объёма (общий и импульсы не трогаем) */
        else if (control == HCTRL_TOTAL_RESET)
        {
            if (password == menu_pass[MENUPASS_SYSTEM])
            {
                totalnl = 0;
                critical_payload_t cp = {
                    .totalnl = 0,
                    .gtotalnl = gtotalnl,
                    .runtime_device = runtime_device,
                    .runtime_flowmeter = runtime_flowmeter,
                    .impulses = impulses,
                    .poweron_counter = poweron_counter};
                critical_save(&cp, false); /* зафиксировать сброс сразу */
            }
            else
                accessError(&status, &acces_deadline);
        }
        /* сброс общего накопленного объёма (рабочий и импульсы не трогаем) */
        else if (control == HCTRL_GTOTAL_RESET)
        {
            if (password == menu_pass[MENUPASS_SYSTEM])
            {
                gtotalnl = 0;
                critical_payload_t cp = {
                    .totalnl = 0,
                    .gtotalnl = 0,
                    .runtime_device = runtime_device,
                    .runtime_flowmeter = runtime_flowmeter,
                    .impulses = impulses,
                    .poweron_counter = poweron_counter};
                critical_save(&cp, false); /* зафиксировать сброс сразу */
            }
            else
                accessError(&status, &acces_deadline);
        }
        /* сервисный сброс: обнуление накопителей объёма, импульсов и таймеров */
        else if (control == HCTRL_SERVICE_RESET)
        {
            if (password == menu_pass[MENUPASS_SERVICE])
            {
                totalnl = 0;
                gtotalnl = 0;
                impulses = 0;
                runtime_device = 0;
                runtime_flowmeter = 0;
                poweron_counter = 0;
                critical_payload_t cp = {
                    .totalnl = 0,
                    .gtotalnl = 0,
                    .runtime_device = 0,
                    .runtime_flowmeter = 0,
                    .impulses = 0,
                    .poweron_counter = 0};
                critical_save(&cp, false); /* зафиксировать сброс сразу */
            }
            else
                accessError(&status, &acces_deadline);
        }
        /* возврат заводских настроек (накопители НЕ трогаются —
         * для этого HCTRL_SERVICE_RESET) */
        else if (control == HCTRL_DEFAULT)
        {
            if (password == menu_pass[MENUPASS_SERVICE])
            {
                settings_load_factory();              /* значения из дескрипторов */
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
