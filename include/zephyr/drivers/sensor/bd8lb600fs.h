/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BD8LB600FS_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BD8LB600FS_H_

#include <zephyr/drivers/sensor.h>

enum sensor_channel_bd8lb600fs {
	/**
	 * Open load detected,
	 * boolean with one bit per output
	 */
	SENSOR_CHAN_BD8LB600FS_OPEN_LOAD = SENSOR_ATTR_PRIV_START,
	/**
	 * Over current protection or thermal shutdown,
	 * boolean with one bit per output
	 */
	SENSOR_CHAN_BD8LB600FS_OVER_CURRENT_OR_THERMAL_SHUTDOWN,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BD8LB600FS_H_ */
