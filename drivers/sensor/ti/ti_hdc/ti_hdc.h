/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TI_HDC_TI_HDC_H_
#define ZEPHYR_DRIVERS_SENSOR_TI_HDC_TI_HDC_H_

#include <zephyr/kernel.h>

#define TI_HDC_REG_TEMP		0x0
#define TI_HDC_REG_HUMIDITY	0x1
#define TI_HDC_REG_CONFIG	0x2
#define TI_HDC_REG_MANUFID	0xFE
#define TI_HDC_REG_DEVICEID	0xFF

#define TI_HDC_MANUFID		0x5449
#define TI_HDC1050_DEVID	0x1050
#define TI_HDC1000_DEVID	0x1000

#define CHT8305_MANUFID		0x5959
#define CHT8305_DEVID		0x8305

#define TI_HDC_REG_CONFIG_MODE		BIT(12)
#define TI_HDC_REG_CONFIG_HEATER	BIT(13)
#define CHT8305_REG_CONFIG_CLKSTR	BIT(14)

/* For 14bit conversion RH needs 6.5ms and Temp 6.35ms */
#define HDC_CONVERSION_TIME	13
/* CHT8305C reference code requires more than 20ms */
#define CHT8305_CONVERSION_TIME	23

enum ti_hdc_variant {
	TI_HDC_INVALID = 0,
	TI_HDC_HDC,
	TI_HDC_CHT8305,
	TI_HDC_MAX,
};

struct ti_hdc_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec drdy;
	enum ti_hdc_variant variant;
	bool heater;
	bool disable_clock_stretching;
};

struct ti_hdc_data {
	uint16_t t_sample;
	uint16_t rh_sample;

	struct gpio_callback gpio_cb;
	struct k_sem data_sem;
	const struct device *dev;
};

#endif
