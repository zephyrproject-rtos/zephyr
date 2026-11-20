/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Attribute value types for the sensor API
 * @ingroup sensor_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ATTRIBUTE_TYPES_H
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ATTRIBUTE_TYPES_H

#include <zephyr/dsp/types.h>
#include <zephyr/dsp/print_format.h>

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup sensor_interface
 * @{
 */

/**
 * Used by the following channel/attribute pairs:
 * - SENSOR_CHAN_ACCEL_XYZ
 *   - SENSOR_ATTR_OFFSET
 * - SENSOR_CHAN_GYRO_XYZ
 *   - SENSOR_ATTR_OFFSET
 * - SENSOR_CHAN_MAGN_XYZ
 *   - SENSOR_ATTR_OFFSET
 */
struct sensor_three_axis_attribute {
	/**
	 * Scaling of the q31 values: a value @c v stands for v * 2^(shift - 31) in the unit of
	 * the channel.
	 */
	int8_t shift;
	/** Values, per axis or as an array. */
	union {
		/** Values per axis. */
		struct {
			q31_t x; /**< X-axis value. */
			q31_t y; /**< Y-axis value. */
			q31_t z; /**< Z-axis value. */
		};
		/** X, Y and Z values. */
		q31_t values[3];
	};
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ATTRIBUTE_TYPES_H */
