/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of TCS3400 sensor
 * @ingroup tcs3400_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS3400_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS3400_H_

/**
 * @brief AMS TCS3400 RGBC sensor
 * @defgroup tcs3400_interface TCS3400
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for TCS3400
 */
enum sensor_attribute_tcs3400 {
	/**
	 * Number of RGBC Integration Cycles
	 *
	 * - sensor_value.val1 should be between 1 and 256.
	 */
	SENSOR_ATTR_TCS3400_INTEGRATION_CYCLES = SENSOR_ATTR_PRIV_START,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TCS3400_H_ */
