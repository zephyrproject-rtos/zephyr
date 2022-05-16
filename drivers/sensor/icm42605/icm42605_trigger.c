/*
 * Copyright (c) 2016 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "icm42605.h"
#include "icm42605_setup.h"

LOG_MODULE_DECLARE(ICM42605, CONFIG_SENSOR_LOG_LEVEL);

int icm42605_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct icm42605_data *drv_data = dev->data;
	const struct icm42605_config *cfg = dev->config;

	if (trig->type != SENSOR_TRIG_DATA_READY
	    && trig->type != SENSOR_TRIG_TAP
	    && trig->type != SENSOR_TRIG_DOUBLE_TAP) {
		return -ENOTSUP;
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	if (handler == NULL) {
		icm42605_turn_off_sensor(dev);
		return 0;
	}

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		drv_data->data_ready_handler = handler;
		drv_data->data_ready_trigger = *trig;
	} else if (trig->type == SENSOR_TRIG_TAP) {
		drv_data->tap_handler = handler;
		drv_data->tap_trigger = *trig;
		drv_data->tap_en = true;
	} else if (trig->type == SENSOR_TRIG_DOUBLE_TAP) {
		drv_data->double_tap_handler = handler;
		drv_data->double_tap_trigger = *trig;
		drv_data->tap_en = true;
	} else {
		return -ENOTSUP;
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);

	icm42605_turn_on_sensor(dev);

	return 0;
}

static void icm42605_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct icm42605_data *drv_data =
		CONTAINER_OF(cb, struct icm42605_data, gpio_cb);
	const struct icm42605_config *cfg = drv_data->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	k_sem_give(&drv_data->gpio_sem);
}

static void icm42605_thread_cb(const struct device *dev)
{
	struct icm42605_data *drv_data = dev->data;
	const struct icm42605_config *cfg = dev->config;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	if (drv_data->tap_handler != NULL ||
	    drv_data->double_tap_handler != NULL) {
		icm42605_tap_fetch(dev);
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
}

static void icm42605_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct icm42605_data *drv_data = p1;

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		icm42605_thread_cb(drv_data->dev);
	}
}

int icm42605_init_interrupt(const struct device *dev)
{
	struct icm42605_data *drv_data = dev->data;
	const struct icm42605_config *cfg = dev->config;
	int result = 0;

	if (!device_is_ready(cfg->gpio_int.port)) {
		LOG_ERR("gpio_int gpio not ready");
		return -ENODEV;
	}

	drv_data->dev = dev;

	gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	gpio_init_callback(&drv_data->gpio_cb, icm42605_gpio_callback, BIT(cfg->gpio_int.pin));
	result = gpio_add_callback(cfg->gpio_int.port, &drv_data->gpio_cb);

	if (result < 0) {
		LOG_ERR("Failed to set gpio callback");
		return result;
	}

	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ICM42605_THREAD_STACK_SIZE, icm42605_thread, drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ICM42605_THREAD_PRIORITY), 0, K_NO_WAIT);

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_INACTIVE);

	return 0;
}
