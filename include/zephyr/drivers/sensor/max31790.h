/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX31790_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX31790_H_

#include <zephyr/drivers/sensor.h>

/* MAX31790 specific channels */
enum sensor_channel_max31790 {
	SENSOR_CHAN_MAX31790_FAN_FAULT = SENSOR_CHAN_PRIV_START,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX31790_H_ */
