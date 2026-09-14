#ifndef _INTERPOLATION_H_
#define _INTERPOLATION_H_

#include <stdbool.h>

float interpolationStep(float const xValues[], float const yValues[], int numValues, float pointX, float threshold);
float interpolationLinear(float const xValues[], float const yValues[], int numValues, float pointX, bool trim);
float interpolationSmoothStep(float const xValues[], float const yValues[], int numValues, float pointX, bool trim);
float interpolationCatmullSpline(float const xValues[], float const yValues[], int numValues, float pointX, bool trim);
float interpolationConstrainedSpline(float const xValues[], float const yValues[], int numValues, float pointX, bool trim);

#endif // !_INTERPOLATION_H_