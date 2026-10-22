#ifndef _METER_H_
#define _METER_H_

#include <stdint.h>

extern uint32_t runtime_device;    /* наработка прибора, сек */
extern uint32_t runtime_flowmeter; /* наработка расходомера, сек */
extern uint64_t totalnl;           /* накопленный объём, нл */
extern uint64_t gtotalnl;          /* общий накопленный объём, нл */
extern uint32_t impulses;          /* счётчик импульсов */
extern uint16_t poweron_counter;   /* счётчик включений */

void timer_100ms_callback(void);
void timer_10ms_callback(void);
void BoardTimer2Init(void);

#endif // !_METER_H_
