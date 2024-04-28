/*
 * Copyright (c) 2024 THE ARC GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_DUMMY_SENSOR_H__
#define __TEST_DUMMY_SENSOR_H__

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#define SENSOR_CHANNEL_NUM 2

struct dummy_sensor_data {
	sensor_trigger_handler_t handler;
	struct sensor_value val[SENSOR_CHANNEL_NUM];
};

struct dummy_sensor_config {
	char *i2c_name;
	uint8_t i2c_address;
};

int dummy_sensor_set_value(const struct device *dev, enum sensor_channel chan,
			   struct sensor_value *val);

#endif /* __TEST_DUMMY_SENSOR_H__ */
