#ifndef _POWERMON_H_
#define _POWERMON_H_

#include <stdbool.h>
#include <stdint.h>
#include "ch32v00x_pwr.h"

/* Аппаратный PVD: выход → EXTI Line8, прерывание PVD_IRQn по обоим фронтам.
 * Пороги PLS[2:0] подтверждены измерением на этом экземпляре (±50 мВ,
 * совпадает с даташитом), фронт спада / фронт восстановления:
 *   MODE0 = 2.7/2.85 В   MODE1 = 2.9/3.05 В   MODE2 = 3.15/3.3 В
 *   MODE3 = 3.3/3.5 В    MODE4 = 3.5/3.7 В    MODE5 = 3.7/3.9 В
 *   MODE6 = 3.9/4.1 В    MODE7 = 4.2/4.4 В
 * Прибор питается от 5 В → MODE7: спад 4.2 В, восстановление 4.4 В —
 * с запасом от номинала, при спаде до минимума записи flash (2.7 В)
 * остаётся более миллисекунды. При переходе на рельсу 3.3 В ставить MODE1. */
#define POWERMON_PVD_LEVEL PWR_PVDLevel_MODE7

extern volatile uint8_t powermon_fail_events; /* входы в аварию в этой сессии */
extern volatile uint8_t powermon_debug;       /* биты-источники аварии */

void powermon_init(void);   /* PVD + прерывание */
bool powermon_failed(void); /* true — питание ниже порога */

#endif // _POWERMON_H_
