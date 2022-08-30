/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_DUMMY_SENSOR_H__
#define __TEST_DUMMY_SENSOR_H__

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#define DUMMY_SENSOR_NAME	"dummy_sensor"
#define DUMMY_SENSOR_NAME_NO_TRIG	"dummy_sensor_no_trig"
#define SENSOR_CHANNEL_NUM	5

struct dummy_sensor_data {
	sensor_trigger_handler_t handler;
	struct sensor_value val[SENSOR_CHANNEL_NUM];
};

struct dummy_sensor_config {
	char *i2c_name;
	uint8_t i2c_address;
};

#endif //__TEST_DUMMY_SENSOR_H__
