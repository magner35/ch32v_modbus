#ifndef _EEPROM_H_
#define _EEPROM_H_

#include <stdbool.h>
#include <stdint.h>
#include "ch32v00x.h"

/* Данные, сохраняемые при пропадании питания. Запись занимает 64-байтовый
 * блок flash (быстрая запись целиком), поэтому payload ограничен 48 Б:
 * uint64 — первыми (без внутренней набивки), reserve добивает до конца.
 * Добавляя поле — уменьшайте reserve, размер должен оставаться 48. */
typedef struct
{
    uint64_t totalnl;           /* накопленный объём, нанолитры */
    uint64_t gtotalnl;          /* общий накопленный объём, нанолитры */
    uint32_t runtime_device;    /* наработка прибора, сек */
    uint32_t runtime_flowmeter; /* наработка расходомера, сек */
    uint32_t impulses;          /* счётчик импульсов */
    uint16_t poweron_counter;   /* счётчик включений */
    uint8_t reserve[18];        /* резерв, заполнять нулями */
} critical_payload_t;

/* Маркер в поле seq: последняя запись сделана аварийным сохранением по PVD.
 * Живёт во flash — переживает отключение питания (в отличие от RAM-счётчиков) */
#define CRITICAL_SEQ_PVD_MARK 0x8000u
#define CRITICAL_SEQ_PVD_MARK_INV 0x7FFFu

extern volatile uint8_t critical_debug; /* биты диагностики тракта записи */

bool critical_init(critical_payload_t *data);                      /* старт: скан журнала + обслуживание страницы */
bool critical_save(const critical_payload_t *data, bool pvd_mark); /* запись БЕЗ стирания (для PVD) */
uint16_t critical_seq(void);                                       /* seq последней валидной записи */

#endif // !_EEPROM_H_
