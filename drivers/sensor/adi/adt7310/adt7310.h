/*
 * Copyright (c) 2023 Andriy Gelman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADT7310_H_
#define ZEPHYR_DRIVERS_SENSOR_ADT7310_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

int adt7310_init_interrupt(const struct device *dev);

int adt7310_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

struct adt7310_data {
	int16_t sample;

#ifdef CONFIG_ADT7310_TRIGGER
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t th_handler;
	const struct sensor_trigger *th_trigger;

	const struct device *dev;

#ifdef CONFIG_ADT7310_TRIGGER_OWN_THREAD
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADT7310_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif CONFIG_ADT7310_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
#endif /* CONFIG_ADT7310_TRIGGER */
};

struct adt7310_dev_config {
	struct spi_dt_spec bus;
#ifdef CONFIG_ADT7310_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ADT7310_H_ */
