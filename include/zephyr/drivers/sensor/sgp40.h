/*
 * Copyright (c) 2021, Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of SGP40 sensor
 * @ingroup sgp40_interface
 *
 * This exposes two attributes for the SGP40 which can be used for
 * setting the on-chip Temperature and Humidity compensation parameters.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP40_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP40_H_

/**
 * @defgroup sgp40_interface SGP40
 * @ingroup sensor_interface_ext
 * @brief Sensirion SGP40 gas sensor
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Custom sensor attributes for SGP40
 */
enum sensor_attribute_sgp40 {
	/**
	 * Temperature in degC (only the integer part is used).
	 */
	SENSOR_ATTR_SGP40_TEMPERATURE = SENSOR_ATTR_PRIV_START,
	/**
	 * Relative Humidity in % (only the integer part is used).
	 */
	SENSOR_ATTR_SGP40_HUMIDITY
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP40_H_ */
