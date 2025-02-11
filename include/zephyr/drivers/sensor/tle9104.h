/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TLE9104_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TLE9104_H_

#include <zephyr/drivers/sensor.h>

enum sensor_channel_tle9104 {
	/** Open load detected, boolean with one bit per output */
	SENSOR_CHAN_TLE9104_OPEN_LOAD = SENSOR_ATTR_PRIV_START,
	/** Over current detected, boolean with one bit per output */
	SENSOR_CHAN_TLE9104_OVER_CURRENT,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TLE9104_H_ */
