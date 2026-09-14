#ifndef _FILTER_H_
#define _FILTER_H_

typedef struct
{
    float filtered_value; // Текущее отфильтрованное значение
    float alpha;          // Базовый коэффициент сглаживания
    bool is_adaptive;     // Флаг адаптивного режима
} ema_filter_t;

typedef struct
{
    float *buffer; // Буфер для хранения значений расхода
    uint8_t size;  // Размер буфера (время демпфирования в секундах)
    uint8_t index; // Текущий индекс в буфере
    float sum;     // Сумма значений в буфере
    bool is_reset; // Флаг сброса фильтра
} damping_filter_t;

/*----------------------------------------------------------------------------*/

void emaFilterInit(ema_filter_t *filter, uint16_t damping_time_seconds, bool adaptive);
float emaFilterUpdate(ema_filter_t *filter, float instant_rate);

void dampingFilterInit(damping_filter_t *filter, uint16_t damping_time);
float dampingFilterUpdate(damping_filter_t *filter, float instant_rate);

/*----------------------------------------------------------------------------*/

#endif // _FILTER_H_