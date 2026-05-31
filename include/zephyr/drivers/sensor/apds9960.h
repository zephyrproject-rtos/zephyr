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
 * @brief Extended public API for the APDS9960 ambient light, RGB, proximity and
 *        gesture sensor.
 * @ingroup apds9960_interface
 */

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief APDS9960 ambient light, RGB, proximity and gesture sensor
 * @defgroup apds9960_interface APDS9960
 * @ingroup sensor_interface_ext
 * @{
 */

/** @brief Custom sensor channels for the APDS9960. */
enum sensor_channel_apds9960 {
	/** Detected gesture, reported as an @ref apds9960_gesture value. */
	SENSOR_CHAN_APDS9960_GESTURE = SENSOR_CHAN_PRIV_START,
};

/** @brief Gesture values reported on @ref SENSOR_CHAN_APDS9960_GESTURE. */
enum apds9960_gesture {
	APDS9960_GESTURE_NONE,
	APDS9960_GESTURE_UP,
	APDS9960_GESTURE_DOWN,
	APDS9960_GESTURE_LEFT,
	APDS9960_GESTURE_RIGHT,
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_APDS9960_H_ */
