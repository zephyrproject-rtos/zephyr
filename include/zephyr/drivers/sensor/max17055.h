/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of MAX17055 sensor
 * @ingroup max17055_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX17055_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX17055_H_

/**
 * @defgroup max17055_interface MAX17055
 * @ingroup sensor_interface_ext
 * @brief Maxim Integrated MAX17055 fuel gauge
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor channels for MAX17055
 */
enum sensor_channel_max17055 {
	/** Battery Open Circuit Voltage. */
	SENSOR_CHAN_MAX17055_VFOCV = SENSOR_CHAN_PRIV_START,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX17055_H_ */
