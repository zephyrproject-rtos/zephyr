/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMAG5273_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMAG5273_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/* --- Additional TMAG5273 definitions */

/** Additional channels supported by the TMAG5273 */
enum tmag5273_sensor_channel {
	/**
	 * Magnitude measurement result between two axis in Gs.
	 */
	TMAG5273_CHAN_MAGNITUDE = SENSOR_CHAN_PRIV_START,

	/**
	 * Magnitude measurement MSB as returned by the sensor.
	 */
	TMAG5273_CHAN_MAGNITUDE_MSB,

	/**
	 * Angle result in deg, magnitude result in Gs and magnitude MSB between two axis.
	 */
	TMAG5273_CHAN_ANGLE_MAGNITUDE,
};

/** Additional attributes supported by the TMAG5273 */
enum tmag5273_attribute {
	/**
	 * Define axis relation measurements.
	 * Supported values are:
	 *   - \c TMAG5273_DT_ANGLE_MAG_NONE (0)
	 *   - \c TMAG5273_DT_ANGLE_MAG_XY (1)
	 *   - \c TMAG5273_DT_ANGLE_MAG_YZ (2)
	 *   - \c TMAG5273_DT_ANGLE_MAG_XZ (3)
	 *
	 * Only available if calculation source can be changed during runtime.
	 */
	TMAG5273_ATTR_ANGLE_MAG_AXIS = SENSOR_ATTR_PRIV_START,
};

/**
 * Supported values
 */

#define TMAG5273_ANGLE_CALC_NONE 0
#define TMAG5273_ANGLE_CALC_XY   1
#define TMAG5273_ANGLE_CALC_YZ   2
#define TMAG5273_ANGLE_CALC_XZ   3

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMAG5273_H_ */
