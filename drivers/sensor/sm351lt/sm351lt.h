/*
 * Copyright (c) 2020, Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SM351LT_SM351LT_H_
#define ZEPHYR_DRIVERS_SENSOR_SM351LT_SM351LT_H_

#include <stdint.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>

#define SENSOR_ATTR_SM351LT_TRIGGER_TYPE SENSOR_ATTR_PRIV_START

struct sm351lt_config {
	const char *bus_name;
	gpio_pin_t gpio_pin;
	gpio_dt_flags_t gpio_flags;
};

struct sm351lt_data {
	const struct device *bus;
	bool sample_status;

#ifdef CONFIG_SM351LT_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	uint32_t trigger_type;
	sensor_trigger_handler_t changed_handler;

#if defined(CONFIG_SM351LT_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_SM351LT_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_SM351LT_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_SM351LT_TRIGGER */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SM351LT_SM351LT_H_ */
