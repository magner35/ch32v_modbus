#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include <stdint.h>
#include <stdbool.h>

// timestamp format
typedef uint32_t soft_timer_stamp_t;
#define SOFT_TIMER_STAMP_MAX (UINT32_MAX)

// timestamp format
typedef void (*soft_timer_tick_cb_t)(void);

typedef struct soft_timer
{
    // The timestamp of the last trigger
    soft_timer_stamp_t prev;

    // Accumulated time
    uint32_t acc;

    // Interval time
    uint32_t interval;

    // callback function
    soft_timer_tick_cb_t fn;

} soft_timer_t;

extern soft_timer_stamp_t soft_timer_uptime;

void soft_timer_inc(uint32_t inc_time);
void soft_timer_init(soft_timer_t *p_soft_time,
                     uint32_t interval,
                     soft_timer_tick_cb_t fn);
bool soft_timer_check(soft_timer_t *p_soft_time);
void soft_timer_tick_callback(soft_timer_t *p_soft_time,
                              soft_timer_tick_cb_t fn);
void soft_timer_group_loop(soft_timer_t *p_soft_time, uint32_t len);

#endif // SOFT_TIMER_H
