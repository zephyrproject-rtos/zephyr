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

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AMS TSL2540 ambient light sensor
 * @defgroup tsl2540_interface TSL2540
 * @ingroup sensor_interface_ext_ams
 * @{
 */

/** @brief Custom sensor attributes for TSL2540 */
enum sensor_attribute_tsl2540 {
	/** Sensor integration time, in ms */
	SENSOR_ATTR_INTEGRATION_TIME = SENSOR_ATTR_PRIV_START + 1,
	/** Sensor ALS interrupt persistence filter */
	SENSOR_ATTR_INT_APERS,
	/** Shut down the sensor */
	SENSOR_ATTR_TSL2540_SHUTDOWN_MODE,
	/** Turn on continuous conversion */
	SENSOR_ATTR_TSL2540_CONTINUOUS_MODE,
	/** Turn on continuous conversion without wait */
	SENSOR_ATTR_TSL2540_CONTINUOUS_NO_WAIT_MODE,
};

/** @brief Gain values for the TSL2540 @ref SENSOR_ATTR_GAIN attribute */
enum sensor_gain_tsl2540 {
	TSL2540_SENSOR_GAIN_1_2, /**< Gain 1/2x */
	TSL2540_SENSOR_GAIN_1,   /**< Gain 1x */
	TSL2540_SENSOR_GAIN_4,   /**< Gain 4x */
	TSL2540_SENSOR_GAIN_16,  /**< Gain 16x */
	TSL2540_SENSOR_GAIN_64,  /**< Gain 64x */
	TSL2540_SENSOR_GAIN_128, /**< Gain 128x */
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2540_H_ */
