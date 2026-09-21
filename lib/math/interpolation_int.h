#ifndef _INTERPOLATION_INT_H_
#define _INTERPOLATION_INT_H_

#include <stdbool.h>
#include <stdint.h>

int64_t interpolationCatmullSplineInt64(int64_t const xValues[], int64_t const yValues[], int numValues, int64_t pointX, bool trim);
int64_t interpolationConstrainedSplineInt64(int64_t const xValues[], int64_t const yValues[], int numValues, int64_t pointX, bool trim);
int64_t interpolationLinearInt64(int64_t const xValues[], int64_t const yValues[], int numValues, int64_t pointX, bool trim);

#endif // !_INTERPOLATION_INT_H_