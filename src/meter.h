#ifndef _METER_H_
#define _METER_H_

#include <stdint.h>

extern uint32_t runtime_device; /* наработка прибора, сек */

void timer_100ms_callback(void);
void timer_10ms_callback(void);
void BoardTimer2Init(void);

#endif // !_METER_H_
