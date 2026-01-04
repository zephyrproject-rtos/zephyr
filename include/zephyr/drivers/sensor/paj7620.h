/*
 * Copyright (c) 2025 Paul Timke <ptimkec@live.com>
 * Copyright (c) 2025  Mohamed Haggouna <mohamed.haggouna@gmail.com>
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of PAJ7620 sensor
 * @ingroup paj7620_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_PAJ7620_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_PAJ7620_H_

/**
 * @brief Pixart PAJ7620 gesture sensor
 * @defgroup paj7620_interface PAJ7620
 * @ingroup sensor_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <zephyr/drivers/sensor.h>

/**
 * @name Gesture flags
 * @{
 */
#define PAJ7620_FLAG_GES_UP               BIT(0) /**< "Up" gesture */
#define PAJ7620_FLAG_GES_DOWN             BIT(1) /**< "Down" gesture */
#define PAJ7620_FLAG_GES_LEFT             BIT(2) /**< "Left" gesture */
#define PAJ7620_FLAG_GES_RIGHT            BIT(3) /**< "Right" gesture */
#define PAJ7620_FLAG_GES_FORWARD          BIT(4) /**< "Forward" gesture */
#define PAJ7620_FLAG_GES_BACKWARD         BIT(5) /**< "Backward" gesture */
#define PAJ7620_FLAG_GES_CLOCKWISE        BIT(6) /**< "Clockwise" gesture */
#define PAJ7620_FLAG_GES_COUNTERCLOCKWISE BIT(7) /**< "Counterclockwise" gesture */
#define PAJ7620_FLAG_GES_WAVE             BIT(8) /**< "Wave" gesture */
/** @} */

/**
 * @brief Custom sensor channels for PAJ7620
 */
enum sensor_channel_paj7620 {
	/**
	 * Gesture data (bit mask).
	 *
	 * This channel will contain gesture data as a bitmask where each
	 * set bit represents a detected gesture. The possible gestures
	 * that can be detected along with their corresponding bit are given
	 * by the PAJ7620_FLAG_GES_X macros
	 */
	SENSOR_CHAN_PAJ7620_GESTURES = SENSOR_CHAN_PRIV_START
};

#ifdef CONFIG_PAJ7620_POWER_MODES
/**
 * @brief Custom sensor attributes for PAJ7620
 */
enum sensor_attribute_paj7620 {
	/* Sensor  power Modes
	 *
	 * PAJ7620_NORMAL_MODE
	 * PAJ7620_NORMAL_MODE
	 * See datasheet for more info on different modes.
	 */
	SENSOR_ATTR_POWER_MODE = SENSOR_ATTR_PRIV_START + 1,
};
enum paj7620_pm_mode {
	/*PAJ7620 possible power modes*/
	PAJ7620_NORMAL_MODE = 0,
	PAJ7620_SLEEP_MODE
};
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_PAJ7620_H_ */
