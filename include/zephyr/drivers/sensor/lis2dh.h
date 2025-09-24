/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of LIS2DH sensor
 * @ingroup lis2dh_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS2DH_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS2DH_H_

/**
 * @defgroup lis2dh_interface LIS2DH
 * @ingroup sensor_interface_ext
 * @brief ST Microelectronics LIS2DH 3-axis accelerometer
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * Possible values for @ref SENSOR_ATTR_LIS2DH_SELF_TEST custom attribute.
 */
enum lis2dh_self_test {
	LIS2DH_SELF_TEST_DISABLE = 0,  /**< Self-test disabled */
	LIS2DH_SELF_TEST_POSITIVE = 1, /**< Simulates a positive-direction acceleration */
	LIS2DH_SELF_TEST_NEGATIVE = 2, /**< Simulates a negative-direction acceleration */
};

/**
 * @brief Custom sensor attributes for LIS2DH
 */
enum sensor_attribute_lis2dh {
	/**
	 * Sets the self-test mode.
	 *
	 * Applies an electrostatic force to the sensor to simulate acceleration without
	 * actually moving the device.
	 *
	 * Use a value from @ref lis2dh_self_test, passed in the sensor_value.val1 field.
	 */
	SENSOR_ATTR_LIS2DH_SELF_TEST = SENSOR_ATTR_PRIV_START,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS2DH_H_ */
