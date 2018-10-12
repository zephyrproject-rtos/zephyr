/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HDC1008_HDC1008_H_
#define ZEPHYR_DRIVERS_SENSOR_HDC1008_HDC1008_H_

#include <kernel.h>

#define HDC1008_I2C_ADDRESS	0x40

#define HDC1008_REG_TEMP	0x0
#define HDC1008_REG_HUMIDITY	0x1
#define HDC1000_MANUFID         0xFE
#define HDC1000_DEVICEID        0xFF

struct hdc1008_data {
	struct device *i2c;
	struct device *gpio;
	struct gpio_callback gpio_cb;
	u16_t t_sample;
	u16_t rh_sample;
	struct k_sem data_sem;
};

#define SYS_LOG_DOMAIN "HDC1008"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif
