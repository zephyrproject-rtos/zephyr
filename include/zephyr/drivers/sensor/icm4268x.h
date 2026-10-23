/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for ICM4268X
 * @ingroup icm4268x_interface
 *
 * Pin function configuration via attributes under the current sensor driver abstraction.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ICM4268X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ICM4268X_H_

#include <zephyr/drivers/sensor.h>

/**
 * @addtogroup icm4268x_interface
 * @{
 */

/**
 * @name Pin 9 functions
 *
 * Values for the @ref SENSOR_ATTR_ICM4268X_PIN9_FUNCTION attribute.
 * @{
 */
#define ICM4268X_PIN9_FUNCTION_INT2  0 /**< Interrupt 2 output */
#define ICM4268X_PIN9_FUNCTION_FSYNC 1 /**< Frame synchronization input */
#define ICM4268X_PIN9_FUNCTION_CLKIN 2 /**< External clock input */
/** @} */

/**
 * @brief Extended sensor attributes for ICM4268X
 *
 * Attributes for setting pin function.
 */
enum sensor_attribute_icm4268x {
	/** Function of pin 9, one of the ICM4268X_PIN9_FUNCTION_* values */
	SENSOR_ATTR_ICM4268X_PIN9_FUNCTION = SENSOR_ATTR_PRIV_START
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ICM4268X_H_ */
