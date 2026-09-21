/*
 * Created on Sun Sep 20 2026
 *
 * Copyright (c) 2026 by magner mr.jaedong@gmail.com
 */

#include "interpolation_int.h"
/*----------------------------------------------------------------------------*/

/**
 * Линейная интерполяция на 64-битной целочисленной арифметике
 *
 * ВАЖНО: Все входные значения должны быть в формате с фиксированной точкой.
 * Например, если вы хотите хранить значения с точностью до 0.001,
 * умножьте все значения на 1000 перед передачей в функцию.
 *
 * Формула: y = y[i] + (y[i+1] - y[i]) * (x - x[i]) / (x[i+1] - x[i])
 *
 * @param xValues Массив X координат (int64_t, фиксированная точка)
 * @param yValues Массив Y координат (int64_t, фиксированная точка)
 * @param numValues Количество точек в массиве
 * @param pointX X координата для интерполяции (int64_t, фиксированная точка)
 * @param trim Если true, обрезать значения за пределами диапазона
 * @return Интерполированное значение Y (int64_t, фиксированная точка)
 */
int64_t interpolationLinearInt64(
    int64_t const xValues[],
    int64_t const yValues[],
    int numValues,
    int64_t pointX,
    bool trim)
{
    // Обработка граничных условий
    if (trim)
    {
        if (pointX <= xValues[0])
            return yValues[0];
        if (pointX >= xValues[numValues - 1])
            return yValues[numValues - 1];
    }

    int i = 0;

    // Экстраполяция за левой границей
    if (pointX <= xValues[0])
    {
        i = 0;
    }
    // Экстраполяция за правой границей
    else if (pointX >= xValues[numValues - 1])
    {
        i = numValues - 2;
    }
    // Поиск нужного интервала
    else
    {
        while (i < numValues - 2 && pointX >= xValues[i + 1])
            i++;
    }

    // Вычисление разностей
    int64_t dx = xValues[i + 1] - xValues[i];
    int64_t dy = yValues[i + 1] - yValues[i];
    int64_t x_diff = pointX - xValues[i];

    // Защита от деления на ноль (если две точки имеют одинаковый X)
    if (dx == 0)
        return yValues[i];

    // Линейная интерполяция: y = y[i] + dy * (x - x[i]) / dx
    // ВАЖНО: Порядок операций критичен для избежания переполнения!
    // Сначала умножение, потом деление для сохранения точности
    int64_t rst = yValues[i] + (dy * x_diff) / dx;

    return rst;
}

// Масштаб для фиксированной точки (24 бита для дробной части)
#define FP_SHIFT 24
#define FP_ONE (1LL << FP_SHIFT)

// ============================================================================
// Catmull-Rom Spline
// ============================================================================

int64_t interpolationCatmullSplineInt64(
    int64_t const xValues[],
    int64_t const yValues[],
    int numValues,
    int64_t pointX,
    bool trim)
{
    // Обработка граничных условий
    if (trim)
    {
        if (pointX <= xValues[0])
            return yValues[0];
        if (pointX >= xValues[numValues - 1])
            return yValues[numValues - 1];
    }

    int i = 0;

    // Экстраполяция за левой границей
    if (pointX <= xValues[0])
    {
        i = 0;
    }
    // Экстраполяция за правой границей
    else if (pointX >= xValues[numValues - 1])
    {
        i = numValues - 2;
    }
    // Поиск нужного интервала
    else
    {
        while (i < numValues - 2 && pointX >= xValues[i + 1])
            i++;
    }

    if (pointX == xValues[i + 1])
        return yValues[i + 1];

    // Вычисление разностей
    int64_t x0 = xValues[i];
    int64_t x1 = xValues[i + 1];
    int64_t y0 = yValues[i];
    int64_t y1 = yValues[i + 1];
    int64_t dx = x1 - x0;
    int64_t x_diff = pointX - x0;

    if (dx == 0)
        return y0;

    // Вычисляем t в формате fixed-point: t = x_diff / dx
    int64_t t = (x_diff << FP_SHIFT) / dx;

    // Вычисляем t² и t³ в fixed-point
    int64_t t2 = (t * t) >> FP_SHIFT;
    int64_t t3 = (t2 * t) >> FP_SHIFT;

    // Коэффициенты Эрмита в fixed-point
    int64_t h00 = 2 * t3 - 3 * t2 + FP_ONE;
    int64_t h10 = t3 - 2 * t2 + t;
    int64_t h01 = -2 * t3 + 3 * t2;
    int64_t h11 = t3 - t2;

    // Вычисление наклонов m0 и m1 в зависимости от позиции
    int64_t m0, m1;

    if (i == 0)
    {
        // Граничный случай: первая точка
        int64_t dx0 = xValues[1] - xValues[0];
        int64_t dy0 = yValues[1] - yValues[0];
        m0 = (dy0 << FP_SHIFT) / dx0;

        int64_t dx1 = xValues[2] - xValues[0];
        int64_t dy1 = yValues[2] - yValues[0];
        m1 = (dy1 << FP_SHIFT) / dx1;
    }
    else if (i == numValues - 2)
    {
        // Граничный случай: последняя точка
        int64_t dx0 = xValues[numValues - 1] - xValues[numValues - 3];
        int64_t dy0 = yValues[numValues - 1] - yValues[numValues - 3];
        m0 = (dy0 << FP_SHIFT) / dx0;

        int64_t dx1 = xValues[numValues - 1] - xValues[numValues - 2];
        int64_t dy1 = yValues[numValues - 1] - yValues[numValues - 2];
        m1 = (dy1 << FP_SHIFT) / dx1;
    }
    else
    {
        // Общий случай
        int64_t dx0 = xValues[i + 1] - xValues[i - 1];
        int64_t dy0 = yValues[i + 1] - yValues[i - 1];
        m0 = (dy0 << FP_SHIFT) / dx0;

        int64_t dx1 = xValues[i + 2] - xValues[i];
        int64_t dy1 = yValues[i + 2] - yValues[i];
        m1 = (dy1 << FP_SHIFT) / dx1;
    }

    // Вычисляем результат, избегая переполнения
    // Формула: rst = h00 * y0 + h10 * dx * m0 + h01 * y1 + h11 * dx * m1
    // Все коэффициенты h** в fixed-point, m0/m1 тоже в fixed-point

    // Сначала вычисляем произведения с промежуточным делением
    int64_t term0 = (h00 * y0) >> FP_SHIFT;
    int64_t term1 = ((h10 * dx) >> FP_SHIFT) * m0 >> FP_SHIFT;
    int64_t term2 = (h01 * y1) >> FP_SHIFT;
    int64_t term3 = ((h11 * dx) >> FP_SHIFT) * m1 >> FP_SHIFT;

    int64_t rst = term0 + term1 + term2 + term3;

    return rst;
}

// ============================================================================
// Constrained Spline
// ============================================================================

// Вспомогательная функция: первая производная в fixed-point
static int64_t getFirstDerivateInt64(
    int64_t const x[],
    int64_t const y[],
    int n,
    int i)
{
    int64_t fd1_x;

    if (i == 0)
    {
        int64_t dx = x[1] - x[0];
        int64_t dy = y[1] - y[0];
        int64_t term1 = (3 * dy << FP_SHIFT) / (2 * dx);
        int64_t term2 = getFirstDerivateInt64(x, y, n, 1) >> 1;
        fd1_x = term1 - term2;
    }
    else if (i == n)
    {
        int64_t dx = x[n] - x[n - 1];
        int64_t dy = y[n] - y[n - 1];
        int64_t term1 = (3 * dy << FP_SHIFT) / (2 * dx);
        int64_t term2 = getFirstDerivateInt64(x, y, n, n - 1) >> 1;
        fd1_x = term1 - term2;
    }
    else
    {
        int64_t dx1 = x[i + 1] - x[i];
        int64_t dy1 = y[i + 1] - y[i];
        int64_t dx2 = x[i] - x[i - 1];
        int64_t dy2 = y[i] - y[i - 1];

        // Проверка знака: (dx1/dy1) * (dx2/dy2) < 0
        int64_t prod_x = dx1 * dx2;
        int64_t prod_y = dy1 * dy2;

        if ((prod_x > 0 && prod_y < 0) || (prod_x < 0 && prod_y > 0))
        {
            fd1_x = 0;
        }
        else
        {
            // fd1_x = 2 / ((dx1/dy1) + (dx2/dy2))
            // = 2 * dy1 * dy2 / (dx1 * dy2 + dx2 * dy1)
            int64_t numerator = 2 * dy1 * dy2;
            int64_t denominator = dx1 * dy2 + dx2 * dy1;

            if (denominator == 0)
                fd1_x = 0;
            else
                fd1_x = (numerator << FP_SHIFT) / denominator;
        }
    }

    return fd1_x;
}

// Вспомогательная функция: левая вторая производная в fixed-point
static int64_t getLeftSecondDerivateInt64(
    int64_t const x[],
    int64_t const y[],
    int n,
    int i)
{
    int64_t fdi_x = getFirstDerivateInt64(x, y, n, i);
    int64_t fdi_xl1 = getFirstDerivateInt64(x, y, n, i - 1);
    int64_t dx = x[i] - x[i - 1];
    int64_t dy = y[i] - y[i - 1];

    // fd2l_x = -2 * (fdi_x + 2 * fdi_xl1) / dx + 6 * dy / dx²
    int64_t term1 = (-2 * (fdi_x + 2 * fdi_xl1)) / dx;
    int64_t term2 = (6 * dy << FP_SHIFT) / (dx * dx);

    int64_t fd2l_x = term1 + term2;

    return fd2l_x;
}

// Вспомогательная функция: правая вторая производная в fixed-point
static int64_t getRightSecondDerivateInt64(
    int64_t const x[],
    int64_t const y[],
    int numValues,
    int i)
{
    int64_t fdi_x = getFirstDerivateInt64(x, y, numValues, i);
    int64_t fdi_xl1 = getFirstDerivateInt64(x, y, numValues, i - 1);
    int64_t dx = x[i] - x[i - 1];
    int64_t dy = y[i] - y[i - 1];

    // fd2r_x = 2 * (2 * fdi_x + fdi_xl1) / dx - 6 * dy / dx²
    int64_t term1 = (2 * (2 * fdi_x + fdi_xl1)) / dx;
    int64_t term2 = (6 * dy << FP_SHIFT) / (dx * dx);

    int64_t fd2r_x = term1 - term2;

    return fd2r_x;
}

int64_t interpolationConstrainedSplineInt64(
    int64_t const xValues[],
    int64_t const yValues[],
    int numValues,
    int64_t pointX,
    bool trim)
{
    // Обработка граничных условий
    if (trim)
    {
        if (pointX <= xValues[0])
            return yValues[0];
        if (pointX >= xValues[numValues - 1])
            return yValues[numValues - 1];
    }

    int i = 0;

    // Экстраполяция за левой границей
    if (pointX <= xValues[0])
    {
        i = 0;
    }
    // Экстраполяция за правой границей
    else if (pointX >= xValues[numValues - 1])
    {
        i = numValues - 2;
    }
    // Поиск нужного интервала
    else
    {
        while (i < numValues - 2 && pointX >= xValues[i + 1])
            i++;
    }

    if (pointX == xValues[i + 1])
        return yValues[i + 1];

    int64_t x0 = xValues[i + 1];
    int64_t x1 = xValues[i];
    int64_t y0 = yValues[i + 1];
    int64_t y1 = yValues[i];

    int64_t dx = x0 - x1;
    if (dx == 0)
        return y1;

    // Вычисляем вторые производные в fixed-point
    int64_t fd2i_xl1 = getLeftSecondDerivateInt64(xValues, yValues, numValues - 1, i + 1);
    int64_t fd2i_x = getRightSecondDerivateInt64(xValues, yValues, numValues - 1, i + 1);

    // Вычисляем коэффициенты полинома в fixed-point
    // d = (fd2i_x - fd2i_xl1) / (6 * dx)
    int64_t d = (fd2i_x - fd2i_xl1) / (6 * dx);

    // c = (x0 * fd2i_xl1 - x1 * fd2i_x) / (2 * dx)
    // Чтобы избежать переполнения, делим каждое произведение отдельно
    int64_t c = ((x0 / dx) * fd2i_xl1 - (x1 / dx) * fd2i_x) / 2;

    // b = (y0 - y1 - c * (x0² - x1²) - d * (x0³ - x1³)) / dx
    // x0² - x1² = (x0 - x1) * (x0 + x1) = dx * (x0 + x1)
    // x0³ - x1³ = (x0 - x1) * (x0² + x0*x1 + x1²) = dx * (x0² + x0*x1 + x1²)
    int64_t x0_plus_x1 = x0 + x1;
    int64_t x0_sq_plus_x0x1_plus_x1_sq = (x0 / dx) * x0 + x0 * (x1 / dx) + (x1 / dx) * x1;

    int64_t b = ((y0 - y1) / dx) - (c * x0_plus_x1 >> FP_SHIFT) - (d * x0_sq_plus_x0x1_plus_x1_sq >> FP_SHIFT);

    // a = y1 - b * x1 - c * x1² - d * x1³
    // Вычисляем с промежуточными делениями
    int64_t x1_scaled = x1 >> (FP_SHIFT / 2);
    int64_t a = y1 - (b * x1_scaled >> (FP_SHIFT / 2)) - ((c * ((x1 / dx) * x1)) >> FP_SHIFT) - ((d * ((x1 / dx) * x1 * (x1 / dx))) >> FP_SHIFT);

    // Вычисляем результат: y = a + pointX * (b + pointX * (c + pointX * d))
    // Используем схему Горнера с промежуточными делениями
    int64_t inner = c + ((pointX / dx) * d >> FP_SHIFT);
    int64_t middle = b + ((pointX / dx) * inner >> FP_SHIFT);
    int64_t rst = a + ((pointX / dx) * middle >> FP_SHIFT);

    return rst;
}