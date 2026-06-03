/*
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for AMS's TSL2540 ambient light sensor
 * @ingroup tsl2540_interface
 *
 * This exposes attributes for the TSL2540 which can be used for
 * setting the on-chip gain and integration time parameters.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2540_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2540_H_

/**
 * @brief AMS TSL2540 ambient light sensor
 * @defgroup tsl2540_interface TSL2540
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Extended sensor attributes for the TSL2540 ambient light sensor. */
enum sensor_attribute_tsl2540 {
	/** Sensor integration time, in ms. */
	SENSOR_ATTR_INTEGRATION_TIME = SENSOR_ATTR_PRIV_START + 1,
	/** Sensor ALS interrupt persistence filter. */
	SENSOR_ATTR_INT_APERS,
	/** Shutdown the sensor. */
	SENSOR_ATTR_TSL2540_SHUTDOWN_MODE,
	/** Turn on continuous conversion. */
	SENSOR_ATTR_TSL2540_CONTINUOUS_MODE,
	/** Turn on continuous conversion without wait. */
	SENSOR_ATTR_TSL2540_CONTINUOUS_NO_WAIT_MODE,
};

/** @brief ADC gain settings for the TSL2540 ambient light sensor. */
enum sensor_gain_tsl2540 {
	TSL2540_SENSOR_GAIN_1_2, /**< 1/2x gain. */
	TSL2540_SENSOR_GAIN_1,   /**< 1x gain. */
	TSL2540_SENSOR_GAIN_4,   /**< 4x gain. */
	TSL2540_SENSOR_GAIN_16,  /**< 16x gain. */
	TSL2540_SENSOR_GAIN_64,  /**< 64x gain. */
	TSL2540_SENSOR_GAIN_128, /**< 128x gain. */
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2540_H_ */
