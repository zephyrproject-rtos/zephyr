/*
 * Copyright (c) 2019 Kevin Townsend
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SIMACCEL_SIMACCEL_H_
#define ZEPHYR_DRIVERS_SENSOR_SIMACCEL_SIMACCEL_H_

#include <zephyr/types.h>
#include <device.h>

struct simaccel_data {
    /** Accelerometer measurement (+/-G). Valid values are 2, 4, 8 or 16. */
    u8_t range;
	/** Raw x-axis data. */
	s16_t xdata;
    /** Raw y-axis data. */
    s16_t ydata;
    /** Raw z-axis data. */
    s16_t zdata;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SIMACCEL_SIMACCEL_H_ */
