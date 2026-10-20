/*
 * Created on Mon Oct 19 2026
 *
 * Copyright (c) 2026 by magner mr.jaedong@gmail.com
 */

#include "settings.h"
#include <stddef.h>

/* Таблицы линеаризации — массивы, в SETTINGS_TABLE не входят */
float lpoint_forward_frequency[LPOINTS];
float lpoint_forward_value[LPOINTS];

/* Эффективное число точек линеаризации (результат settingsLpoints) */
uint8_t lpoints_count = 0;

/* Рабочая копия настроек */
settings_t g_settings;

#define DESC_ROW(n, t, mn, mx, dv) ST_DESC_##t(n, mn, mx, dv)
const setting_desc_t g_settings_desc[SETTINGS_COUNT] =
    {
        SETTINGS_TABLE(DESC_ROW)

};

#define OFFS_ROW(n, t, mn, mx, dv) offsetof(settings_t, n),
const uint16_t g_settings_offs[SETTINGS_COUNT] =
    {
        SETTINGS_TABLE(OFFS_ROW)

};

void meterInit(void)
{
    if (!settings_load_flash())
    {
        settings_load_factory();
    }
    /* TODO: применить к железу — адрес/скорость/чётность RS485, пороги фильтра датчика */
}

void settings_load_factory(void)
{
    for (uint8_t i = 0; i < SETTINGS_COUNT; i++)
    {
        const setting_desc_t *d = &g_settings_desc[i];
        void *field = (uint8_t *)&g_settings + g_settings_offs[i];
        switch ((setting_type_t)d->type)
        {
        case ST_FLOAT:
            *(float *)field = d->lim.f.def;
            break;
        case ST_U32:
            *(uint32_t *)field = d->lim.u32.def;
            break;
        case ST_U16:
            *(uint16_t *)field = d->lim.u16.def;
            break;
        case ST_I8:
            *(int8_t *)field = d->lim.i8.def;
            break;
        case ST_U8:
            *(uint8_t *)field = d->lim.u8.def;
            break;
        case ST_ENUM:
            *(uint8_t *)field = d->lim.e.def;
            break;
        case ST_BOOL:
            *(bool *)field = (d->lim.b.def != 0);
            break;
        }
    }
}

bool settings_check(uint8_t idx, float val)
{
    if (idx >= SETTINGS_COUNT)
        return false;
    const setting_desc_t *d = &g_settings_desc[idx];

    switch ((setting_type_t)d->type)
    {
    case ST_FLOAT:
        return (val == val); /* без ограничений, отсекаем только NaN */
    case ST_BOOL:
        return (val == 0.0f) || (val == 1.0f);
    case ST_ENUM:
        return (val >= 0.0f) && (val <= (float)(d->lim.e.count - 1u));
    case ST_U8:
        return (val >= (float)d->lim.u8.min) && (val <= (float)d->lim.u8.max);
    case ST_I8:
        return (val >= (float)d->lim.i8.min) && (val <= (float)d->lim.i8.max);
    case ST_U16:
        return (val >= (float)d->lim.u16.min) && (val <= (float)d->lim.u16.max);
    case ST_U32:
        return (val >= (float)d->lim.u32.min) && (val <= (float)d->lim.u32.max);
    }
    return false;
}

float settings_get(uint8_t idx)
{
    if (idx >= SETTINGS_COUNT)
        return 0.0f;
    const setting_desc_t *d = &g_settings_desc[idx];
    const void *field = (const uint8_t *)&g_settings + g_settings_offs[idx];
    switch ((setting_type_t)d->type)
    {
    case ST_FLOAT:
        return *(const float *)field;
    case ST_U32:
        return (float)*(const uint32_t *)field;
    case ST_U16:
        return (float)*(const uint16_t *)field;
    case ST_I8:
        return (float)*(const int8_t *)field;
    case ST_U8:
    case ST_ENUM:
        return (float)*(const uint8_t *)field;
    case ST_BOOL:
        return (float)*(const bool *)field;
    }
    return 0.0f;
}

bool settings_set(uint8_t idx, float val)
{
    if (!settings_check(idx, val))
        return false;
    const setting_desc_t *d = &g_settings_desc[idx];
    void *field = (uint8_t *)&g_settings + g_settings_offs[idx];
    switch ((setting_type_t)d->type)
    {
    case ST_FLOAT:
        *(float *)field = val;
        break;
    case ST_U32:
        *(uint32_t *)field = (uint32_t)val;
        break;
    case ST_U16:
        *(uint16_t *)field = (uint16_t)val;
        break;
    case ST_I8:
        *(int8_t *)field = (int8_t)val;
        break;
    case ST_U8:
        *(uint8_t *)field = (uint8_t)val;
        break;
    case ST_ENUM:
        *(uint8_t *)field = (uint8_t)val;
        break;
    case ST_BOOL:
        *(bool *)field = (val != 0.0f);
        break;
    }
    return true;
}

uint8_t settingsLpoints(void)
{
    /* 1. Заполненный префикс: первая незаполненная точка (частота <= 0 или
     *    NaN) обрывает таблицу, она и все последующие не учитываются. */
    uint8_t n = 0;
    while (n < LPOINTS && lpoint_forward_frequency[n] > 0.0f)
        n++;

    /* 2. Нормализация: частоты строго возрастают, дубликаты недопустимы.
     *    Минимальный шаг — 1 Гц. Неиспользованный хвост не трогаем. */
    for (uint8_t i = 1; i < n; i++)
    {
        if (lpoint_forward_frequency[i] <= lpoint_forward_frequency[i - 1])
            lpoint_forward_frequency[i] = lpoint_forward_frequency[i - 1] + 1.0f;
    }

    /* 3. Линеаризация требует минимум две точки, иначе — выключена */
    lpoints_count = (n >= 2) ? n : 0;
    return lpoints_count;
}

bool settings_save_flash(void)
{
    /* TODO: ping-pong двух страниц, seq/crc32, стирание без длительного запрета IRQ */
    return false;
}

bool settings_load_flash(void)
{
    /* TODO: проверить обе страницы, выбрать валидную с наибольшим seq */
    return false;
}
