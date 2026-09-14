/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public header for TCS34725 color sensor.
 * @ingroup sensor_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS34725_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS34725_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Driver-specific sensor channel identifiers for the TCS34725.
 *
 * Extends the Zephyr sensor_channel enum starting from SENSOR_CHAN_PRIV_START
 * to avoid conflicts with the standard Zephyr sensor channels defined in
 * <zephyr/drivers/sensor.h>.
 *
 * These channels expose derived values computed from the raw clear, red,
 * green, and blue ADC readings using the ams DN40 algorithm:
 *
 *   - SENSOR_CHAN_TCS34725_LUX        - illuminance in lux
 *   - SENSOR_CHAN_TCS34725_COLOR_TEMP - correlated color temperature in Kelvin
 *
 * @note VALIDITY FLAG: For both of these channels, sensor_channel_get()
 * returns the result in val->val1, and a validity flag in val->val2:
 *   - val->val2 == 1: the DN40 calculation succeeded; val->val1 holds a
 *     genuine, trustworthy measurement.
 *   - val->val2 == 0: the calculation could not produce a reliable result
 *     (RGBC channel saturation, clear channel below the usable minimum,
 *     or - most commonly - highly saturated/pure single-color light that
 *     falls outside the DN40 formula's valid input domain). In this case
 *     val->val1 is always 0 and MUST NOT be interpreted as a real
 *     measurement (it does not mean "zero lux" or "0 Kelvin").
 *
 * Applications should always check val->val2 before using val->val1 for
 * these two channels. The raw SENSOR_CHAN_RED/GREEN/BLUE/LIGHT channels
 * are always valid and do not use val->val2 for anything (it is always 0
 * for those channels).
 *
 * Use these identifiers with sensor_channel_get() after a successful
 * sensor_sample_fetch() call.
 */
enum tcs34725_channel {
	/** Calculated illuminance (lux), from the DN40 algorithm. */
	SENSOR_CHAN_TCS34725_LUX = SENSOR_CHAN_PRIV_START,

	/** Calculated correlated color temperature (CT), in Kelvin. */
	SENSOR_CHAN_TCS34725_COLOR_TEMP,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS34725_H_ */
