/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file drivers/sensor_v2.h
 *
 * @brief Public APIs for the sensor driver.
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_SENSOR_UTILS_H_
#define INCLUDE_ZEPHYR_DRIVERS_SENSOR_UTILS_H_

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_H_
#error "Please include drivers/sensor.h directly"
#endif

#include <zephyr/math/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_G FLOAT_TO_FP(9.80665f)
#define SENSOR_PI FLOAT_TO_FP(3.141592f)

/**
 * @brief Convert m/s^2 to gravity units (g)
 *
 * @param ms2 The measurement in meters / second^2
 * @return The same value in g
 */
static inline fp_t sensor_ms2_to_g(fp_t ms2)
{
	return fp_div(ms2, SENSOR_G);
}

/**
 * @brief Convert gravity units (g) to m/s^2
 *
 * @param g The measurement in g
 * @return The same value in meters / second^2
 */
static inline fp_t sensor_g_to_ms2(fp_t g)
{
	return fp_mul(g, SENSOR_G);
}

/**
 * @brief Convert radians to degrees
 *
 * @param rad The measurement in radians
 * @return The same value in degrees
 */
static inline fp_t sensor_rad_to_degrees(fp_t rad)
{
	return fp_div(fp_mul(rad, FLOAT_TO_FP(180.0f)), SENSOR_PI);
}

/**
 * @brief Convert degrees to radians
 *
 * @param degrees The measurement in degrees
 * @return The same value in radians
 */
static inline fp_t sensor_degrees_to_rad(fp_t degrees)
{
	return fp_div(fp_mul(degrees, SENSOR_PI), FLOAT_TO_FP(180.0f));
}

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZEPHYR_DRIVERS_SENSOR_UTILS_H_ */
