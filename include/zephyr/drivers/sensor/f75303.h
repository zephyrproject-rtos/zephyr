/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of F75303 sensor
 * @ingroup f75303_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_F75303_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_F75303_H_

/**
 * @brief Fintek F75303 temperature sensor
 * @defgroup f75303_interface F75303
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor channels for F75303
 */
enum sensor_channel_f75303 {
	/** Temperature of remote sensor 1 */
	SENSOR_CHAN_F75303_REMOTE1 = SENSOR_CHAN_PRIV_START,
	/** Temperature of remote sensor 2 */
	SENSOR_CHAN_F75303_REMOTE2,
};

/**
 * @}
 */

#endif
