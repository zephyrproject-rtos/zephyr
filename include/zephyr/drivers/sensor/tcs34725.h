/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of TCS34725 sensor
 * @ingroup tcs34725_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS34725_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS34725_H_

/**
 * @brief AMS TCS34725 RGBC color sensor
 * @defgroup tcs34725_interface TCS34725
 * @ingroup sensor_interface_ext_ams
 * @{
 */

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Custom sensor channels for TCS34725
 *
 * Values derived from the raw clear, red, green and blue readings using the ams DN40
 * algorithm, read with sensor_channel_get() after a successful sensor_sample_fetch().
 *
 * For these channels, @c sensor_value.val2 is a validity flag, not a fractional part:
 *
 * - 1: the DN40 calculation succeeded and @c sensor_value.val1 holds the measurement.
 * - 0: the calculation could not produce a reliable result, because an RGBC channel is
 *   saturated, the clear channel is below the usable minimum, or the light is a highly
 *   saturated single color outside the DN40 input domain. @c sensor_value.val1 is then 0 and
 *   must not be read as "0 lux" or "0 K".
 *
 * The raw @ref SENSOR_CHAN_RED, @ref SENSOR_CHAN_GREEN, @ref SENSOR_CHAN_BLUE and
 * @ref SENSOR_CHAN_LIGHT channels are always valid and their @c sensor_value.val2 is always 0.
 */
enum tcs34725_channel {
	/**
	 * Illuminance, from the DN40 algorithm.
	 *
	 * - @c sensor_value.val1 is the illuminance, in lux.
	 * - @c sensor_value.val2 is the validity flag.
	 */
	SENSOR_CHAN_TCS34725_LUX = SENSOR_CHAN_PRIV_START,

	/**
	 * Correlated color temperature, from the DN40 algorithm.
	 *
	 * - @c sensor_value.val1 is the color temperature, in Kelvin.
	 * - @c sensor_value.val2 is the validity flag.
	 */
	SENSOR_CHAN_TCS34725_COLOR_TEMP,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS34725_H_ */
