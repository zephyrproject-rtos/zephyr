/*
 * Copyright (c) 2019 Actinius
 * Copyright (c) 2025 Bang & Olufsen A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_OPT300X_H_
#define ZEPHYR_DRIVERS_SENSOR_OPT300X_H_

#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#define OPT300X_REG_RESULT 0x00
#define OPT300X_REG_CONFIG 0x01
#define OPT300X_REG_LOW_LIMIT 0x02
#define OPT300X_REG_MANUFACTURER_ID 0x7E
#define OPT300X_REG_DEVICE_ID 0x7F

#define OPT300X_MANUFACTURER_ID_VALUE 0x5449
#define OPT300X_DEVICE_ID_VALUE 0x3001

#define OPT300X_CONVERSION_MODE_MASK (BIT(10) | BIT(9))
#define OPT300X_CONVERSION_MODE_CONTINUOUS (BIT(10) | BIT(9))

#define OPT300X_LIMIT_EXPONENT_MASK (BIT(15) | BIT(14) | BIT(13) | BIT(12))
#define OPT300X_LIMIT_EXPONENT_DEFAULT 0x0000

#define OPT300X_SAMPLE_EXPONENT_SHIFT 12
#define OPT300X_MANTISSA_MASK 0xfff

#ifdef CONFIG_OPT300X_TRIGGER
int opt300x_init_interrupt(const struct device *dev);

int opt300x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int opt300x_reg_update(const struct device *dev, uint8_t reg, uint16_t mask, uint16_t val);

int opt300x_reg_read(const struct device *dev, uint8_t reg, uint16_t *val);

#endif

struct opt300x_data {
	uint16_t sample;

#ifdef CONFIG_OPT300X_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb_int;

	const struct sensor_trigger *trigger;
	sensor_trigger_handler_t handler;
	struct k_mutex handler_mutex;

#if defined(CONFIG_OPT300X_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_OPT300X_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_OPT300X_TRIGGER_GLOBAL_THREAD)
	struct k_work work_int;
#endif

#endif
};

struct opt300x_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_OPT300X_TRIGGER
	const struct gpio_dt_spec gpio_int;
#endif
};

#endif /* _SENSOR_OPT300X_ */
