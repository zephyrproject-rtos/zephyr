/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of LSM6DSVXXX sensor
 * @ingroup lsm6dsvxxx_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LSM6DSVXXX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LSM6DSVXXX_H_

/**
 * @defgroup lsm6dsvxxx_interface LSM6DSVXXX
 * @ingroup sensor_interface_ext
 * @brief ST Microelectronics LSM6DSVXXX 3-axis IMU family
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for LSM6DSVXXX
 */
enum sensor_attribute_lsm6dsvxxx {
	/**
	 * Gets the self-test mode result.
	 * The result is stored in the sensor_value.val1 field.
	 * See @ref lsm6dsvxxx_self_test_result for possible values.
	 */
	SENSOR_ATTR_GET_SELF_TEST_RESULT = SENSOR_ATTR_PRIV_START,
};

/** @brief Self-test result for the LSM6DSVXXX. */
enum lsm6dsvxxx_self_test_result {
	LSM6DSVXXX_ST_OK = 0,   /**< Self-test passed. */
	LSM6DSVXXX_ST_FAIL = 1, /**< Self-test failed. */
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_LSM6DSVXXX_H_ */
