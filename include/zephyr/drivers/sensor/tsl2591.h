/*
 * Copyright (c) 2023 Kurtis Dinelle
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for AMS's TSL2591 ambient light sensor
 * @ingroup tsl2591_interface
 *
 * This exposes attributes for the TSL2591 which can be used for
 * setting the on-chip gain, integration time, and persist filter parameters.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2591_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2591_H_

/**
 * @brief AMS TSL2591 ambient light sensor
 * @defgroup tsl2591_interface TSL2591
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Extended sensor attributes for the TSL2591 ambient light sensor. */
enum sensor_attribute_tsl2591 {
	/** Sensor ADC Gain Mode.
	 * Rather than set this value directly, can only be set to operate in one of four modes:
	 *
	 * TSL2591_SENSOR_GAIN_LOW
	 * TSL2591_SENSOR_GAIN_MED
	 * TSL2591_SENSOR_GAIN_HIGH
	 * TSL2591_SENSOR_GAIN_MAX
	 *
	 * See datasheet for actual typical gain scales these modes correspond to.
	 */
	SENSOR_ATTR_GAIN_MODE = SENSOR_ATTR_PRIV_START + 1,

	/** Sensor ADC Integration Time (in ms)
	 * Can only be set to one of six values:
	 *
	 * 100, 200, 300, 400, 500, or 600
	 */
	SENSOR_ATTR_INTEGRATION_TIME,

	/** Sensor ALS Interrupt Persist Filter.
	 * Represents the number of consecutive sensor readings outside of a set threshold
	 * before triggering an interrupt. Can only be set to one of sixteen values:
	 *
	 * 0, 1, 2, 3, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, or 60
	 *
	 * Setting this to 0 causes an interrupt to generate every ALS cycle,
	 * regardless of threshold.
	 * Setting this to 1 is equivalent to the no-persist interrupt mode.
	 */
	SENSOR_ATTR_INT_PERSIST
};

/** @brief ADC gain modes for the TSL2591 ambient light sensor. */
enum sensor_gain_tsl2591 {
	TSL2591_SENSOR_GAIN_LOW,  /**< Low gain mode. */
	TSL2591_SENSOR_GAIN_MED,  /**< Medium gain mode. */
	TSL2591_SENSOR_GAIN_HIGH, /**< High gain mode. */
	TSL2591_SENSOR_GAIN_MAX   /**< Maximum gain mode. */
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2591_H_ */
