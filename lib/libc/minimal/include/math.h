/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/* Dummy math.h to fulfill the requirements of certain libraries.
 */

#ifdef __cplusplus
extern "C" {
#endif

double ceil(double x);
float ceilf(float x);
double floor(double x);
double log(double x);
double sqrt(double x);

#ifdef __cplusplus
}
#endif
