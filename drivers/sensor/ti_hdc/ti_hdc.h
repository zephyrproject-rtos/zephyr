/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TI_HDC_TI_HDC_H_
#define ZEPHYR_DRIVERS_SENSOR_TI_HDC_TI_HDC_H_

#include <zephyr/kernel.h>

#define TI_HDC_REG_TEMP	0x0
#define TI_HDC_REG_HUMIDITY	0x1
#define TI_HDC_REG_MANUFID	0xFE
#define TI_HDC_REG_DEVICEID	0xFF

#define TI_HDC_MANUFID	0x5449
#define TI_HDC1050_DEVID	0x1050
#define TI_HDC1000_DEVID	0x1000

/* For 14bit conversion RH needs 6.5ms and Temp 6.35ms */
#define HDC_CONVERSION_TIME     13

struct ti_hdc_config {
	struct i2c_dt_spec i2c;
#if DT_INST_NODE_HAS_PROP(0, drdy_gpios)
	struct gpio_dt_spec drdy;
#endif
};

struct ti_hdc_data {
	uint16_t t_sample;
	uint16_t rh_sample;

#if DT_INST_NODE_HAS_PROP(0, drdy_gpios)
	struct gpio_callback gpio_cb;
	struct k_sem data_sem;
	const struct device *dev;
#endif  /* DT_INST_NODE_HAS_PROP(0, drdy_gpios) */
};

#endif
