#include "soft_timer.h"

soft_timer_stamp_t soft_timer_uptime = 0;
bool soft_timer_uptime_overflow = 0;

#define SOFT_TIMER_CB_NONE ((soft_timer_tick_cb_t)0)

/*******************************************************************************
 * @brief Software timer absolute timestamp increments
 * @param inc_time increased time
 * @return None
 ******************************************************************************/
inline void soft_timer_inc(uint32_t inc_time)
{
    soft_timer_uptime += inc_time;
}

/*******************************************************************************
 * @brief Initialize software timer
 * @param p_soft_time software timer structure pointer
 * @param interval time interval
 * @param fn callback function
 * @return None
 ******************************************************************************/
void soft_timer_init(soft_timer_t *p_soft_time,
                     uint32_t interval,
                     soft_timer_tick_cb_t fn)
{
    p_soft_time->interval = interval;
    p_soft_time->prev = soft_timer_uptime;
    p_soft_time->fn = fn;
}

/*******************************************************************************
 * @brief determines whether the target time interval is reached
 * @param p_soft_time software timer structure pointer
 * @return true: arrival time false: no arrival time
 ******************************************************************************/
bool soft_timer_check(soft_timer_t *p_soft_time)
{
    // time change
    if (soft_timer_uptime == p_soft_time->prev)
        return false;

    // Determine whether there is overflow and accumulate the interval time
    if (p_soft_time->prev > soft_timer_uptime)
    {
        p_soft_time->acc += (uint64_t)soft_timer_uptime +
                            SOFT_TIMER_STAMP_MAX + 1 -
                            p_soft_time->prev;
    }
    else
    {
        p_soft_time->acc += soft_timer_uptime - p_soft_time->prev;
    }

    // Reset timer timestamp
    p_soft_time->prev = soft_timer_uptime;

    // Determine whether the accumulated time reaches the set time, and reset the accumulated time
    if (p_soft_time->acc >= p_soft_time->interval)
    {
        p_soft_time->acc = 0;

        // Check whether the function pointer is null
        if (p_soft_time->fn != SOFT_TIMER_CB_NONE)
        {
            p_soft_time->fn();
        }
        return true;
    }

    return false;
}

/*******************************************************************************
 * @brief determines whether the target time interval is reached
 * @param p_soft_time software timer structure pointer
 * @param fn callback function
 * @return None
 ******************************************************************************/
void soft_timer_tick_callback(soft_timer_t *p_soft_time, soft_timer_tick_cb_t fn)
{
    if (!soft_timer_check(p_soft_time))
        return;

    // Check whether the function pointer is null
    if (fn == SOFT_TIMER_CB_NONE)
        return;

    fn();
}

/************************************************************************************
 * @brief timer array loop processing function
 * @param p_soft_time software timer group first address
 * @param len The number of timers in the group
 * @return None
 ******************************************************************************/
void soft_timer_group_loop(soft_timer_t *p_soft_time, uint32_t len)
{
    uint32_t i = 0;
    for (i = 0; i < len; i++)
    {
        soft_timer_check(p_soft_time);
    }
}
