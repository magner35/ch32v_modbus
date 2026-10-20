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
