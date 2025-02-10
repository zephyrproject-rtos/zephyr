/*
 * Copyright (c) 2024 Paul Timke <ptimkec@live.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended Public API for PAJ7620 sensor
 *
 * Some capabilities of the sensor cannot be expressed
 * within the sensor driver abstraction
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_PAJ7620_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_PAJ7620_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <zephyr/drivers/sensor.h>

enum paj7620_gesture {
	PAJ7620_GES_NONE = 0,         /* No gesture */
	PAJ7620_GES_UP,               /* Upwards gesture */
	PAJ7620_GES_DOWN,	      /* Downward gesture */
	PAJ7620_GES_LEFT,             /* Leftward gesture */
	PAJ7620_GES_RIGHT,            /* Rightward gesture */
	PAJ7620_GES_FORWARD,          /* Forward gesture */
	PAJ7620_GES_BACKWARD,         /* Backward gesture */
	PAJ7620_GES_CLOCKWISE,        /* Clockwise circular gesture */
	PAJ7620_GES_COUNTERCLOCKWISE, /* Anticlockwise circular gesture */
	PAJ7620_GES_WAVE              /* Wave gesture */
};

enum sensor_attribute_paj7620 {
	/**
	 * Time in milliseconds (ms) from which a first gesture is detected
	 * to give time to detect a second intended gesture.
	 * The interrupt pin will go active when a gesture is first detected, but
	 * if the user is trying to move their hand to do a Z-axis gesture
	 * (backward, forward), they will first trigger a lateral (up, down,
	 * left, right) gesture, which will immediately raise the interrupt.
	 */
	SENSOR_ATTR_PAJ7620_GESTURE_ENTRY_TIME = SENSOR_ATTR_PRIV_START,

	/** Time in milliseconds (ms) for exiting the sensor's field of view */
	SENSOR_ATTR_PAJ7620_GESTURE_EXIT_TIME,
};

enum sensor_channel_paj7620 {
	/** This channel will contain gesture data (out of the 9 gestures) */
	SENSOR_CHAN_PAJ7620_GESTURES = SENSOR_CHAN_PRIV_START
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_PAJ7620_H_ */
