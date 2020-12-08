/*
 * Copyright (c) 2020 Kalyan Sriram <kalyan@coderkalyan.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHR_DRIVERS_SENSOR_HX711_HX711_H_
#define ZEPHR_DRIVERS_SENSOR_HX711_HX711_H_

#define SENSOR_CHAN_FORCE_RAW SENSOR_CHAN_PRIV_START

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <zephyr/types.h>

struct hx711_data {
	const struct device *sck;
	const struct device *dout;

	double data;
	double offset;
	double scale;

#ifdef CONFIG_HX711_TRIGGER
	const struct device *dev;
	struct gpio_callback dout_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_HX711_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_HX711_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem dout_sem;
#elif defined(CONFIG_HX711_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_HX711_TRIGGER */
};

struct hx711_config {
	const char *sck_label;
	uint8_t sck_pin;
	uint8_t sck_flags;

	const char *dout_label;
	uint8_t dout_pin;
	uint8_t dout_flags;
};

#ifdef CONFIG_HX711_TRIGGER
int hx711_trigger_set(const struct device *dev,
		const struct sensor_trigger *trig,
		sensor_trigger_handler_t handler);

int hx711_init_interrupt(const struct device *dev);
#endif

#endif /* __SENSOR_HX711__ */
