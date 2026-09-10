/*
 * Copyright (c) 2025 Carl Zeiss AG
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of S9706 sensor
 * @ingroup s9706_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_S9706_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_S9706_H_

/**
 * @brief Hamamatsu S9706 RGB sensor
 * @defgroup s9706_interface S9706
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for S9706
 */
enum sensor_attribute_s9706 {
	/**
	 * Clock speed when reading out the sensor value in Hz.
	 */
	SENSOR_ATTR_S9706_CLOCK = SENSOR_ATTR_PRIV_START,
	/**
	 * Integration time in microseconds.
	 */
	SENSOR_ATTR_S9706_INTEGRATION_TIME,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_S9706_H_ */
