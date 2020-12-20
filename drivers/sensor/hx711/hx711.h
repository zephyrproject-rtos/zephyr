/*
 * Copyright (c) 2020, George Gkinis
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HX711_H_
#define ZEPHYR_DRIVERS_SENSOR_HX711_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <device.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>

#include <drivers/sensor/hx711.h>

struct hx711_data {
	const struct device *dout_gpio;
	const struct device *sck_gpio;
	const struct device *rate_gpio;

	int32_t reading;

	int offset;
	struct sensor_value slope;
	char gain;
	enum hx711_rate rate;
	enum hx711_power power;
};

struct hx711_config {
	gpio_pin_t dout_pin;
	const char *dout_ctrl;
	gpio_dt_flags_t dout_flags;

	gpio_pin_t sck_pin;
	const char *sck_ctrl;
	gpio_dt_flags_t sck_flags;

	gpio_pin_t rate_pin;
	const char *rate_ctrl;
	gpio_dt_flags_t rate_flags;
};

#ifdef __cplusplus
}
#endif

#endif
