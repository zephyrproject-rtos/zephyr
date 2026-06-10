/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the INA2xx family of current/power monitors.
 * @ingroup ina2xx_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA2XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA2XX_H_

/**
 * @brief TI INA2xx current/power monitor family
 * @defgroup ina2xx_interface INA2XX
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Custom sensor channels for the INA2xx family.
 *
 * Available only on device variants that expose the corresponding register (for example, INA228).
 */
enum sensor_channel_ina2xx {
	/**
	 * Accumulated energy, in joules.
	 *
	 * - @c sensor_value.val1 is the integer part of the energy (J).
	 * - @c sensor_value.val2 is the fractional part (in millionths of a joule).
	 */
	SENSOR_CHAN_INA2XX_ENERGY = SENSOR_CHAN_PRIV_START,

	/**
	 * Accumulated charge, in coulombs.
	 *
	 * - @c sensor_value.val1 is the integer part of the charge (C).
	 * - @c sensor_value.val2 is the fractional part (in millionths of a coulomb).
	 */
	SENSOR_CHAN_INA2XX_CHARGE,
};

/**
 * @brief Extended sensor attributes for the INA2xx family.
 */
enum sensor_attribute_ina2xx {
	/**
	 * ADC configuration register value.
	 *
	 * - @c sensor_value.val1 is the raw 16-bit ADC configuration register.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_ATTR_ADC_CONFIGURATION = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA2XX_H_ */
