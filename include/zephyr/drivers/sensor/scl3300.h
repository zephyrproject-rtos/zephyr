/*
 * Copyright (c) 2026 Sacra Systems Private Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCL3300_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCL3300_H_

#include <zephyr/drivers/sensor.h>

enum scl3300_measurement_mode {
	SCL3300_MODE_1 = 1,
	SCL3300_MODE_2,
	SCL3300_MODE_3,
	SCL3300_MODE_4,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCL3300_H_ */
