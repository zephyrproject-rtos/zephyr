/*
 * Copyright (c) 2025 Paul Timke <ptimkec@live.com>
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

#define PAJ7620_FLAG_GES_UP               BIT(0)
#define PAJ7620_FLAG_GES_DOWN             BIT(1)
#define PAJ7620_FLAG_GES_LEFT             BIT(2)
#define PAJ7620_FLAG_GES_RIGHT            BIT(3)
#define PAJ7620_FLAG_GES_FORWARD          BIT(4)
#define PAJ7620_FLAG_GES_BACKWARD         BIT(5)
#define PAJ7620_FLAG_GES_CLOCKWISE        BIT(6)
#define PAJ7620_FLAG_GES_COUNTERCLOCKWISE BIT(7)
#define PAJ7620_FLAG_GES_WAVE             BIT(8)

enum sensor_channel_paj7620 {
	/**
	 * This channel will contain gesture data as a bitmask where each
	 * set bit represents a detected gesture. The possible gestures
	 * that can be detected along with their corresponding bit are given
	 * by the PAJ7620_FLAG_GES_X macros
	 */
	SENSOR_CHAN_PAJ7620_GESTURES = SENSOR_CHAN_PRIV_START
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_PAJ7620_H_ */
