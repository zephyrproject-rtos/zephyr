/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of Iclegend S3KM1110 sensor
 * @ingroup s3km1110_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_S3KM1110_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_S3KM1110_H_

/**
 * @brief Iclegend S3KM1110 24 GHz mmWave radar
 * @defgroup s3km1110_interface Iclegend S3KM1110
 * @ingroup sensor_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Radar target status values reported by the S3KM1110.
 */
enum s3km1110_target_status {
	S3KM1110_TARGET_NO_TARGET = 0x00, /**< No target detected */
	S3KM1110_TARGET_MOVING = 0x01,    /**< Moving target detected */
	S3KM1110_TARGET_STATIC = 0x02,    /**< Static target detected */
	S3KM1110_TARGET_BOTH = 0x03,      /**< Both moving and static targets detected */
	S3KM1110_TARGET_ERROR = 0x04,     /**< Error in target detection */
};

/**
 * @brief Custom sensor channels for S3KM1110.
 */
enum sensor_channel_s3km1110 {
	/** Target status.
	 *
	 * @c sensor_value.val1 is an @ref s3km1110_target_status value.
	 * @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_CHAN_S3KM1110_TARGET_STATUS = SENSOR_CHAN_PRIV_START,
	/** Distance to moving target in meters.
	 *
	 * @c sensor_value.val1 is the integer part of the distance (meters).
	 * @c sensor_value.val2 is the fractional part (in millionths of a meter).
	 */
	SENSOR_CHAN_S3KM1110_MOVING_DISTANCE,
	/** Distance to static target in meters.
	 *
	 * @c sensor_value.val1 is the integer part of the distance (meters).
	 * @c sensor_value.val2 is the fractional part (in millionths of a meter).
	 */
	SENSOR_CHAN_S3KM1110_STATIC_DISTANCE,
	/** Moving target energy level (in percent).
	 *
	 * @c sensor_value.val1 is the energy value (0–100).
	 * @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_CHAN_S3KM1110_MOVING_ENERGY,
	/** Static target energy level (in percent).
	 *
	 * @c sensor_value.val1 is the energy value (0–100).
	 * @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_CHAN_S3KM1110_STATIC_ENERGY,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_S3KM1110_H_ */
