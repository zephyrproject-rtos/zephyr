/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
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
 * Used by the following channel/attribute pairs:
 * - SENSOR_CHAN_ACCEL_XYZ
 *   - SENSOR_ATTR_OFFSET
 * - SENSOR_CHAN_GYRO_XYZ
 *   - SENSOR_ATTR_OFFSET
 * - SENSOR_CHAN_MAGN_XYZ
 *   - SENSOR_ATTR_OFFSET
 */
struct sensor_three_axis_attribute {
	int8_t shift;
	union {
		struct {
			q31_t x;
			q31_t y;
			q31_t z;
		};
		q31_t values[3];
	};
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ATTRIBUTE_TYPES_H */
