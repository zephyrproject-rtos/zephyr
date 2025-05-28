/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ICM42688_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ICM42688_H_

#include <zephyr/drivers/sensor.h>

/**
 * @file
 * @brief Extended public API for ICM42688
 *
 * Pin function configuration via attributes under the current sensor driver abstraction.
 */

#define ICM42688_PIN9_FUNCTION_INT2  0
#define ICM42688_PIN9_FUNCTION_FSYNC 1
#define ICM42688_PIN9_FUNCTION_CLKIN 2

/**
 * @brief Extended sensor attributes for ICM42688
 *
 * Attributes for setting pin function.
 */
enum sensor_attribute_icm42688 {
	SENSOR_ATTR_ICM42688_PIN9_FUNCTION = SENSOR_ATTR_PRIV_START
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ICM42688_H_ */
