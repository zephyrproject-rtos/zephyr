/*
 * Copyright (c) 2025 PHYTEC America LLC
 * Author: Florijan Plohl <florijan.plohl@norik.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include "icm40627.h"
#include "icm40627_reg.h"
#include "icm40627_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ICM40627, CONFIG_SENSOR_LOG_LEVEL);

static void icm40627_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct icm40627_data *data = CONTAINER_OF(cb, struct icm40627_data, gpio_cb);

#if defined(CONFIG_ICM40627_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ICM40627_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void icm40627_thread_cb(const struct device *dev)
{
	struct icm40627_data *data = dev->data;
	const struct icm40627_config *cfg = dev->config;

	icm40627_lock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	if (data->data_ready_handler) {
		data->data_ready_handler(dev, data->data_ready_trigger);
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	icm40627_unlock(dev);
}

#if defined(CONFIG_ICM40627_TRIGGER_OWN_THREAD)

static void icm40627_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct icm40627_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		icm40627_thread_cb(data->dev);
	}
}

#elif defined(CONFIG_ICM40627_TRIGGER_GLOBAL_THREAD)

static void icm40627_work_handler(struct k_work *work)
{
	struct icm40627_data *data = CONTAINER_OF(work, struct icm40627_data, work);

	icm40627_thread_cb(data->dev);
}

#endif

int icm40627_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	int res = 0;
	struct icm40627_data *data = dev->data;
	const struct icm40627_config *cfg = dev->config;

	if (!handler) {
		return -EINVAL;
	}

	icm40627_lock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		data->data_ready_handler = handler;
		data->data_ready_trigger = trig;
		break;
	default:
		res = -ENOTSUP;
		break;
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	icm40627_unlock(dev);

	return res;
}

int icm40627_trigger_init(const struct device *dev)
{
	struct icm40627_data *data = dev->data;
	const struct icm40627_config *cfg = dev->config;
	int res = 0;

	if (!cfg->gpio_int.port) {
		LOG_ERR("trigger enabled but no interrupt gpio supplied");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		LOG_ERR("gpio_int gpio not ready");
		return -ENODEV;
	}

	data->dev = dev;
	gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	gpio_init_callback(&data->gpio_cb, icm40627_gpio_callback, BIT(cfg->gpio_int.pin));
	res = gpio_add_callback(cfg->gpio_int.port, &data->gpio_cb);

	if (res < 0) {
		LOG_ERR("Failed to set gpio callback");
		return res;
	}

	k_mutex_init(&data->mutex);

#if defined(CONFIG_ICM40627_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_ICM40627_THREAD_STACK_SIZE,
			icm40627_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ICM40627_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ICM40627_TRIGGER_GLOBAL_THREAD)
	data->work.handler = icm40627_work_handler;
#endif

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
}

int icm40627_trigger_enable_interrupt(const struct device *dev)
{
	int res;
	const struct icm40627_config *cfg = dev->config;

	/* pulse-mode (auto clearing), push-pull and active-high */
	res = cfg->bus_io->write(&cfg->bus, REG_INT_CONFIG,
				 BIT_INT1_DRIVE_CIRCUIT | BIT_INT1_POLARITY);

	if (res) {
		return res;
	}

	/* enable data ready interrupt on INT1 pin */
	return cfg->bus_io->write(&cfg->bus, REG_INT_SOURCE0, BIT_INT_DRDY_INT1_EN);
}

void icm40627_lock(const struct device *dev)
{
	struct icm40627_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);
}

void icm40627_unlock(const struct device *dev)
{
	struct icm40627_data *data = dev->data;

	k_mutex_unlock(&data->mutex);
}
