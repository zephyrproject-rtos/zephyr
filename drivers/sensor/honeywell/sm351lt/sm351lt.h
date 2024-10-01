/*
 * Copyright (c) 2020, Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SM351LT_SM351LT_H_
#define ZEPHYR_DRIVERS_SENSOR_SM351LT_SM351LT_H_

#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#define SENSOR_ATTR_SM351LT_TRIGGER_TYPE SENSOR_ATTR_PRIV_START

struct sm351lt_config {
	struct gpio_dt_spec int_gpio;
};

struct sm351lt_data {
	bool sample_status;

#ifdef CONFIG_SM351LT_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	uint32_t trigger_type;
	sensor_trigger_handler_t changed_handler;
	const struct sensor_trigger *changed_trigger;

#if defined(CONFIG_SM351LT_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_SM351LT_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_SM351LT_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_SM351LT_TRIGGER */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SM351LT_SM351LT_H_ */
