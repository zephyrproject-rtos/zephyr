/*
 * Copyright (c) 2019 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_OPT3001_H_
#define ZEPHYR_DRIVERS_SENSOR_OPT3001_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>


#define OPT3001_REG_RESULT 0x00
#define OPT3001_REG_CONFIG 0x01
#define OPT3001_REG_MANUFACTURER_ID 0x7E
#define OPT3001_REG_DEVICE_ID 0x7F

#define OPT3001_MANUFACTURER_ID_VALUE 0x5449
#define OPT3001_DEVICE_ID_VALUE 0x3001

#define OPT3001_CONVERSION_MODE_MASK (BIT(10) | BIT(9))
#define OPT3001_CONVERSION_MODE_CONTINUOUS (BIT(10) | BIT(9))

#define OPT3001_SAMPLE_EXPONENT_SHIFT 12
#define OPT3001_MANTISSA_MASK 0xfff

struct opt3001_config {
	struct gpio_dt_spec irq_spec;
	struct i2c_dt_spec i2c;
};

struct opt3001_data {
	uint16_t sample;
#if defined(CONFIG_OPT3001_TRIGGER)
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t limit_handler;
#endif /* CONFIG_OPT3001_TRIGGER */
#if defined(CONFIG_OPT3001_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_OPT3001_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_OPT3001_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif /* CONFIG_OPT3001_TRIGGER_GLOBAL_THREAD */
};

#if defined(CONFIG_OPT3001_TRIGGER)
int opt3001_trigger_init(const struct device *device);

int opt3001_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
int opt3001_set_th(const struct device *device, uint16_t value, uint8_t addr);
#endif /* CONFIG_OPT3001_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_OPT3001_H_ */
