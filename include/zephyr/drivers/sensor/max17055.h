/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX17055_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX17055_H_

#include <zephyr/drivers/sensor.h>

enum sensor_channel_max17055 {
	/** Battery Open Circuit Voltage. */
	SENSOR_CHAN_MAX17055_VFOCV = SENSOR_CHAN_PRIV_START,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX17055_H_ */
