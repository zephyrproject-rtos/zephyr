/*
 * Copyright (c) 2019 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_OPT3001_H_
#define ZEPHYR_DRIVERS_SENSOR_OPT3001_H_

#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#define OPT3001_REG_RESULT 0x00
#define OPT3001_REG_CONFIG 0x01
#define OPT3001_REG_LOW_LIMIT 0x02
#define OPT3001_REG_MANUFACTURER_ID 0x7E
#define OPT3001_REG_DEVICE_ID 0x7F

#define OPT3001_MANUFACTURER_ID_VALUE 0x5449
#define OPT3001_DEVICE_ID_VALUE 0x3001

#define OPT3001_CONVERSION_MODE_MASK (BIT(10) | BIT(9))
#define OPT3001_CONVERSION_MODE_CONTINUOUS (BIT(10) | BIT(9))

#define OPT3001_LIMIT_EXPONENT_MASK (BIT(15) | BIT(14) | BIT(13) | BIT(12))

#define OPT3001_SAMPLE_EXPONENT_SHIFT 12
#define OPT3001_MANTISSA_MASK 0xfff

struct opt3001_trigger {
	const struct sensor_trigger *trigger;
	sensor_trigger_handler_t handler;
};

struct opt3001_data {
	const struct device *dev;
	uint16_t sample;
	struct gpio_callback gpio_cb_int;
	struct k_work work_int;
	struct opt3001_trigger trig;
};

struct opt3001_config {
	struct i2c_dt_spec i2c;
	const struct gpio_dt_spec gpio_int;
};

#endif /* _SENSOR_OPT3001_ */
