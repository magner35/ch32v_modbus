/*
 * Created on Tue Oct 20 2026
 *
 * Copyright (c) 2026 by magner mr.jaedong@gmail.com
 */

#include "settings.h"

/* Заводские значения и диапазоны собираются из SETTINGS_TABLE:
 * value ← колонка «дефолт», range ← колонка «макс» */
#define FACTORY_ROW(n, t, mn, mx, def) ST_INIT_##t(n, mn, mx, def)

const set_t factory_meter =
    {
        SETTINGS_TABLE(FACTORY_ROW)

};

/* Заводская таблица линеаризации (прямой ход): две точки с K=1 во всём
 * диапазоне — таблица формально валидна, но линеаризация нейтральна.
 * Незаполненный хвост (нули) игнорируется settingsLpoints.
 * Реальные точки калибровки прибора записываются поверх. */
const float factory_lpoint_forward_frequency[LPOINTS] = {10.0f, 20.0f, 40.0f, 80.0f, 160.0f, 320.0f, 0.0f, 0.0f, 0.0f, 0.0f};
const float factory_lpoint_forward_value[LPOINTS] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0.0f, 0.0f, 0.0f, 0.0f};
