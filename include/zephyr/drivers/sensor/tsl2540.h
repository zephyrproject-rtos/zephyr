/*
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for AMS's TSL2540 ambient light sensor
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

enum sensor_attribute_tsl2540 {
	/* Sensor Gain */
	SENSOR_ATTR_GAIN = SENSOR_ATTR_PRIV_START + 1,
	/* Sensor Integration Time (in ms) */
	SENSOR_ATTR_INTEGRATION_TIME,
	/* Sensor ALS interrupt persistence filters */
	SENSOR_ATTR_INT_APERS,
	/* Shutdown the sensor */
	SENSOR_ATTR_TSL2540_SHUTDOWN_MODE,
	/* Turn on continuous conversion */
	SENSOR_ATTR_TSL2540_CONTINUOUS_MODE,
	/* Turn on continuous conversion without wait */
	SENSOR_ATTR_TSL2540_CONTINUOUS_NO_WAIT_MODE,
};

enum sensor_gain_tsl2540 {
	TSL2540_SENSOR_GAIN_1_2,
	TSL2540_SENSOR_GAIN_1,
	TSL2540_SENSOR_GAIN_4,
	TSL2540_SENSOR_GAIN_16,
	TSL2540_SENSOR_GAIN_64,
	TSL2540_SENSOR_GAIN_128,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TSL2540_H_ */
