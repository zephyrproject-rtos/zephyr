/*
 * Copyright (c) 2024, Calian Advanced Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of LM95234 sensor
 * @ingroup lm95234_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LM95234_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LM95234_H_

/**
 * @defgroup lm95234_interface LM95234
 * @ingroup sensor_interface_ext
 * @brief LM95234 Quad Remote Diode and Local Temperature Sensor
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * Custom sensor channels for LM95234
 */
enum sensor_channel_lm95234 {
	/** Temperature of remote diode 1 */
	SENSOR_CHAN_LM95234_REMOTE_TEMP_1 = SENSOR_CHAN_PRIV_START,
	/** Temperature of remote diode 2 */
	SENSOR_CHAN_LM95234_REMOTE_TEMP_2,
	/** Temperature of remote diode 3 */
	SENSOR_CHAN_LM95234_REMOTE_TEMP_3,
	/** Temperature of remote diode 4 */
	SENSOR_CHAN_LM95234_REMOTE_TEMP_4
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_LM95234_H_ */
