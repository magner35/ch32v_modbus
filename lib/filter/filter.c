/*
 * Created on Thu Nov 20 2025
 *
 * Copyright (c) 2025 by magner mr.jaedong@gmail.com
 */

/* libc include files */
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>
/* User files */
#include "filter.h"
/*----------------------------------------------------------------------------*/

void emaFilterInit(ema_filter_t *filter, uint16_t damping_time_seconds, bool adaptive)
{
    // alpha = 2 / (N + 1), где N - период демпфирования в отсчётах
    // При обновлении 1 раз в секунду: alpha = 2 / (damping_time + 1)
    filter->alpha = (damping_time_seconds > 0) ? 2.0f / (damping_time_seconds + 1) : 1.0f;
    filter->filtered_value = 0.0f;
    filter->is_adaptive = adaptive;
}
/*----------------------------------------------------------------------------*/

float emaFilterUpdate(ema_filter_t *filter, float instant_rate)
{
    // Если демпфирование отключено
    if (filter->alpha >= 1.0f)
    {
        filter->filtered_value = instant_rate;
        return instant_rate;
    }
    // Проверка условия сброса (отклонение > 25%)
    if (filter->is_adaptive && filter->filtered_value > 0.0f)
    {
        float deviation = fabs(instant_rate - filter->filtered_value) / filter->filtered_value;
        if (deviation > 0.25f)
        { // 25% порог
            // Резкое изменение - используем мгновенное значение
            filter->filtered_value = instant_rate;
            return instant_rate;
        }
    }
    // Стандартное EMA: filtered = alpha * new + (1 - alpha) * filtered
    filter->filtered_value = filter->alpha * instant_rate + (1.0f - filter->alpha) * filter->filtered_value;
    return filter->filtered_value;
}
/*----------------------------------------------------------------------------*/

void dampingFilterInit(damping_filter_t *filter, float *buffer, uint8_t damping_time)
{
    /* Буфер предоставляет вызывающий (без heap: pvPortMalloc из времён
     * FreeRTOS убран). Время жизни буфера должно перекрывать жизнь фильтра.
     * damping_time == 0 или buffer == NULL — фильтр выключен (update
     * возвращает вход без изменений). */
    filter->buffer = buffer;
    filter->size = (buffer != NULL) ? damping_time : 0;
    filter->index = 0;
    filter->sum = 0.0f;
    filter->is_reset = true;

    for (uint8_t i = 0; i < filter->size; i++)
    {
        filter->buffer[i] = 0.0f;
    }
}
/*----------------------------------------------------------------------------*/

float dampingFilterUpdate(damping_filter_t *filter, float instant_rate)
{
    // Если демпфирование отключено или фильтр не инициализирован
    if (filter->size == 0 || filter->buffer == NULL)
    {
        return instant_rate;
    }
    // Если фильтр в режиме сброса, заполняем буфер текущим значением
    if (filter->is_reset)
    {
        for (uint8_t i = 0; i < filter->size; i++)
        {
            filter->buffer[i] = instant_rate;
        }
        filter->sum = instant_rate * filter->size; /* эквивалент по-элементного суммирования */
        filter->is_reset = false;
        filter->index = 0;
        return instant_rate;
    }
    // Вычисляем текущее среднее значение
    float current_average = filter->sum / filter->size;
    // Проверяем условие сброса (отклонение > 25%)
    if (current_average > 0.0f && fabs(instant_rate - current_average) > (0.25f * current_average))
    {
        filter->is_reset = true;
        return instant_rate;
    }
    // Обновляем кольцевой буфер
    filter->sum -= filter->buffer[filter->index];       // Вычитаем старое значение
    filter->buffer[filter->index] = instant_rate;       // Записываем новое значение
    filter->sum += instant_rate;                        // Добавляем новое значение к сумме
    filter->index = (filter->index + 1) % filter->size; // Увеличиваем индекс

    return filter->sum / filter->size; // Возвращаем новое среднее значение
}
/*----------------------------------------------------------------------------*/
