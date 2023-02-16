/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_CHX01_CHX01_COMMON_H
#define ZEPHYR_DRIVERS_SENSOR_CHX01_CHX01_COMMON_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include "soniclib.h"

struct chx01_common_config {
	struct i2c_dt_spec i2c;
	const struct gpio_dt_spec gpio_int;
	const struct gpio_dt_spec gpio_program;
	const struct gpio_dt_spec gpio_reset;
};

enum default_firmware {
	DEFAULT_FIRMWARE_NONE,
	DEFAULT_FIRMWARE_GPR,
	DEFAULT_FIRMWARE_GPR_SR,
};

struct ch101_data {
	const struct device *dev;
	ch_dev_t ch_driver;
	ch_group_t ch_group;
	int64_t range_um;
};

struct ch101_config {
	struct chx01_common_config common_config;
	enum default_firmware default_firmware;
};

static inline struct chx01_common_config *get_common_config(ch_dev_t *dev_ptr)
{
	if (dev_ptr->part_number == CH101_PART_NUMBER) {
		struct ch101_data *data = CONTAINER_OF(dev_ptr, struct ch101_data, ch_driver);

		return &((struct ch101_config*)data->dev->config)->common_config;
	}
	return NULL;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_CHX01_CHX01_COMMON_H */
