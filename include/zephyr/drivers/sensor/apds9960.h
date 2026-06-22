/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_APDS9960_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_APDS9960_H_

/**
 * @file
 * @brief Header file for extended sensor API of APDS9960 sensor
 * @ingroup apds9960_interface
 */

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Avago APDS9960 ambient light, RGB, proximity and gesture sensor
 * @defgroup apds9960_interface APDS9960
 * @ingroup sensor_interface_ext
 * @{
 */

/**
 * @brief Custom sensor channels for the APDS9960.
 */
enum sensor_channel_apds9960 {
	/**
	 * Most recently detected gesture.
	 *
	 * - @c sensor_value.val1 is the gesture, one of the @ref apds9960_gesture values.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_CHAN_APDS9960_GESTURE = SENSOR_CHAN_PRIV_START,
};

/**
 * @brief Gesture values reported in @c sensor_value.val1 of the @ref SENSOR_CHAN_APDS9960_GESTURE
 * channel.
 */
enum apds9960_gesture {
	/** No gesture detected. */
	APDS9960_GESTURE_NONE,
	/** Upward gesture (hand moving up across the sensor). */
	APDS9960_GESTURE_UP,
	/** Downward gesture (hand moving down across the sensor). */
	APDS9960_GESTURE_DOWN,
	/** Leftward gesture (hand moving left across the sensor). */
	APDS9960_GESTURE_LEFT,
	/** Rightward gesture (hand moving right across the sensor). */
	APDS9960_GESTURE_RIGHT,
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_APDS9960_H_ */
