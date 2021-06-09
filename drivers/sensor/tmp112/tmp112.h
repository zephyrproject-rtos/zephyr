/*
 * Copyright (c) 2020 Innoseis BV
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP112_TMP112_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP112_TMP112_H_

#include <device.h>

struct tmp112_data {
	int16_t sample;
};

struct tmp112_config {
	const struct device *bus;
	uint16_t i2c_slv_addr;
};

#endif
