/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the ST VL53L4CD time-of-flight sensor.
 * @ingroup sensor_interface
 *
 * Standard channel decoded by the driver:
 *  - SENSOR_CHAN_DISTANCE: meters, Q31 with shift VL53L4CD_DISTANCE_Q31_SHIFT.
 *
 * Driver specific channels (SENSOR_CHAN_VL53L4CD_*) are plain integers
 * encoded as Q31 with shift VL53L4CD_RAW_Q31_SHIFT.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L4CD_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L4CD_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Driver-specific sensor channels for the VL53L4CD.
 *
 * Diagnostic values reported with each ranging result, useful for judging
 * distance quality. All are plain integers encoded as Q31 with shift
 * VL53L4CD_RAW_Q31_SHIFT; use VL53L4CD_RAW_Q31_TO_INT() to decode them.
 *
 * @note Trust a distance reading only when SENSOR_CHAN_VL53L4CD_RANGE_STATUS
 * equals VL53L4CD_RANGE_STATUS_VALID.
 */
enum sensor_channel_vl53l4cd {
	/** Range status (VL53L4CD_RANGE_STATUS_*), integer. */
	SENSOR_CHAN_VL53L4CD_RANGE_STATUS = SENSOR_CHAN_PRIV_START,

	/** Number of enabled SPADs, integer. */
	SENSOR_CHAN_VL53L4CD_SPAD_COUNT,

	/** Signal rate in kcps, integer. */
	SENSOR_CHAN_VL53L4CD_SIGNAL_RATE,

	/** Ambient rate in kcps, integer. */
	SENSOR_CHAN_VL53L4CD_AMBIENT_RATE,

	/** Signal rate per SPAD in kcps, integer. */
	SENSOR_CHAN_VL53L4CD_SIGNAL_PER_SPAD,

	/** Ambient rate per SPAD in kcps, integer. */
	SENSOR_CHAN_VL53L4CD_AMBIENT_PER_SPAD,

	/** Sigma (range noise estimate) in mm, integer. */
	SENSOR_CHAN_VL53L4CD_SIGMA,
};

/**
 * @brief Driver-specific sensor attributes for the VL53L4CD.
 *
 * Used with sensor_attr_set() and sensor_attr_get() on SENSOR_CHAN_DISTANCE
 * or SENSOR_CHAN_ALL. The value is passed in sensor_value.val1.
 *
 * @note Changing an attribute briefly stops ranging while the new timing is
 * programmed. The inter-measurement period must be 0 (continuous) or greater
 * than the timing budget.
 */
enum sensor_attribute_vl53l4cd {
	/** Timing budget in ms (10..200), sensor_value.val1. */
	SENSOR_ATTR_VL53L4CD_TIMING_BUDGET = SENSOR_ATTR_PRIV_START,

	/**
	 * Inter-measurement period in ms, sensor_value.val1.
	 * 0 = continuous mode, otherwise must be greater than the timing budget.
	 */
	SENSOR_ATTR_VL53L4CD_INTER_MEASUREMENT,
};

/** Range status value meaning a valid measurement. */
#define VL53L4CD_RANGE_STATUS_VALID 0

/** Range status value meaning no / unknown status (see ST UM2931 for the others). */
#define VL53L4CD_RANGE_STATUS_UNKNOWN 255

/** Q31 shift used for SENSOR_CHAN_DISTANCE (value in meters). */
#define VL53L4CD_DISTANCE_Q31_SHIFT 7

/** Q31 shift used for the SENSOR_CHAN_VL53L4CD_* integer channels. */
#define VL53L4CD_RAW_Q31_SHIFT 20

/**
 * @brief Convert a decoded SENSOR_CHAN_DISTANCE Q31 value to millimeters.
 *
 * The result is rounded to the nearest millimeter.
 *
 * @param q Decoded Q31 distance value (meters, shift VL53L4CD_DISTANCE_Q31_SHIFT).
 *
 * @return Distance in millimeters.
 */
#define VL53L4CD_DISTANCE_Q31_TO_MM(q)                                                             \
	((int32_t)((((int64_t)(q) * 1000) * (1LL << VL53L4CD_DISTANCE_Q31_SHIFT) + (1LL << 30)) >> \
		   31))

/**
 * @brief Convert a decoded SENSOR_CHAN_VL53L4CD_* Q31 value to an integer.
 *
 * The result is rounded to the nearest integer.
 *
 * @param q Decoded Q31 value (shift VL53L4CD_RAW_Q31_SHIFT).
 *
 * @return Integer value of the channel (status code, count, rate in kcps or mm).
 */
#define VL53L4CD_RAW_Q31_TO_INT(q)                                                                 \
	((int32_t)(((int64_t)(q) + (1LL << (30 - VL53L4CD_RAW_Q31_SHIFT))) >>                      \
		   (31 - VL53L4CD_RAW_Q31_SHIFT)))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L4CD_H_ */
