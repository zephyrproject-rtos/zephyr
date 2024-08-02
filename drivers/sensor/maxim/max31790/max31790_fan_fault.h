/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MAX31790_MAX31790_FAN_FAULT_H_
#define ZEPHYR_DRIVERS_SENSOR_MAX31790_MAX31790_FAN_FAULT_H_

#include <zephyr/drivers/i2c.h>

struct max31790_fan_fault_config {
	struct i2c_dt_spec i2c;
};

struct max31790_fan_fault_data {
	uint16_t value;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_MAX31790_MAX31790_FAN_FAULT_H_ */
