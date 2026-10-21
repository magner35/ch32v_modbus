#ifndef _EEPROM_H_
#define _EEPROM_H_

#include <stdbool.h>
#include <stdint.h>
#include "ch32v00x.h"

/* Данные, сохраняемые при пропадании питания */
typedef struct
{
    uint32_t runtime_device; /* наработка прибора, сек */
} critical_payload_t;

/* Маркер в поле seq: последняя запись сделана аварийным сохранением по PVD.
 * Живёт во flash — переживает отключение питания (в отличие от RAM-счётчиков) */
#define CRITICAL_SEQ_PVD_MARK     0x8000u
#define CRITICAL_SEQ_PVD_MARK_INV 0x7FFFu

extern volatile uint8_t critical_debug; /* биты диагностики тракта записи */

bool critical_init(critical_payload_t *data); /* старт: скан журнала + обслуживание страницы */
bool critical_save(const critical_payload_t *data, bool pvd_mark); /* запись БЕЗ стирания (для PVD) */
uint16_t critical_seq(void);                      /* seq последней валидной записи */

#endif // !_EEPROM_H_
