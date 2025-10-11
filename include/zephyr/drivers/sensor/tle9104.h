/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of TLE9104 sensor
 * @ingroup tle9104_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TLE9104_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TLE9104_H_

/**
 * @defgroup tle9104_interface TLE9104
 * @ingroup sensor_interface_ext
 * @brief TLE9104 4-channel powertrain switch
 * @{
 */

#include <zephyr/drivers/sensor.h>

/** Custom sensor channels for TLE9104 */
enum sensor_channel_tle9104 {
	/**
	 * Open load detected
	 *
	 * Boolean with one bit per output (e.g. if sensor_value.val1 == 0b0101, then open load has
	 * been detected on OUT1 and OUT3)
	 */
	SENSOR_CHAN_TLE9104_OPEN_LOAD = SENSOR_ATTR_PRIV_START,
	/**
	 * Over current detected
	 *
	 * Boolean with one bit per output (e.g. if sensor_value.val1 == 0b0110, then over current
	 * has been detected on OUT2 and OUT3)
	 */
	SENSOR_CHAN_TLE9104_OVER_CURRENT,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TLE9104_H_ */
