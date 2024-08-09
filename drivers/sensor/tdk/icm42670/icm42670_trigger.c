/*
 * Copyright (c) 2024 TDK Invensense
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2016 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include "icm42670.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ICM42670, CONFIG_SENSOR_LOG_LEVEL);

int icm42670_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct icm42670_data *data = dev->data;
	const struct icm42670_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	if (handler == NULL) {
		return 0;
	}

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		data->data_ready_handler = handler;
		data->data_ready_trigger = trig;
#ifdef CONFIG_TDK_APEX
	} else if (trig->type == SENSOR_TRIG_MOTION) {
		data->data_ready_handler = handler;
		data->data_ready_trigger = trig;
#endif
	} else {
		return -ENOTSUP;
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

static void icm42670_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct icm42670_data *data = CONTAINER_OF(cb, struct icm42670_data, gpio_cb);
	const struct icm42670_config *cfg = data->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

#if defined(CONFIG_ICM42670_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ICM42670_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void icm42670_thread_cb(const struct device *dev)
{
	struct icm42670_data *data = dev->data;
	const struct icm42670_config *cfg = dev->config;

	if (data->data_ready_handler != NULL) {
		data->data_ready_handler(dev, data->data_ready_trigger);
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
}

#if defined(CONFIG_ICM42670_TRIGGER_OWN_THREAD)

static void icm42670_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct icm42670_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		icm42670_thread_cb(data->dev);
	}
}
#elif defined(CONFIG_ICM42670_TRIGGER_GLOBAL_THREAD)

static void icm42670_work_handler(struct k_work *work)
{
	struct icm42670_data *data = CONTAINER_OF(work, struct icm42670_data, work);

	icm42670_thread_cb(data->dev);
}

#endif

int icm42670_trigger_init(const struct device *dev)
{
	struct icm42670_data *data = dev->data;
	const struct icm42670_config *cfg = dev->config;
	int result = 0;

	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		LOG_ERR("gpio_int gpio not ready");
		return -ENODEV;
	}

	data->dev = dev;

	gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	gpio_init_callback(&data->gpio_cb, icm42670_gpio_callback, BIT(cfg->gpio_int.pin));
	result = gpio_add_callback(cfg->gpio_int.port, &data->gpio_cb);

	if (result < 0) {
		LOG_ERR("Failed to set gpio callback");
		return result;
	}

	k_mutex_init(&data->mutex);

#if defined(CONFIG_ICM42670_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_ICM42670_THREAD_STACK_SIZE,
			icm42670_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ICM42670_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ICM42670_TRIGGER_GLOBAL_THREAD)
	data->work.handler = icm42670_work_handler;
#endif

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_INACTIVE);

	return 0;
}

void icm42670_lock(const struct device *dev)
{
	struct icm42670_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);
}

void icm42670_unlock(const struct device *dev)
{
	struct icm42670_data *data = dev->data;

	k_mutex_unlock(&data->mutex);
}
