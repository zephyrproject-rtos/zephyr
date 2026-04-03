/*
 * Copyright (c) 2023 Andriy Gelman <andriy.gelman@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "adt7310.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ADT7310, CONFIG_SENSOR_LOG_LEVEL);

static void adt7310_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				  uint32_t pins)
{
	struct adt7310_data *drv_data = CONTAINER_OF(cb, struct adt7310_data, gpio_cb);

#if defined(CONFIG_ADT7310_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADT7310_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

#if defined(CONFIG_ADT7310_TRIGGER_OWN_THREAD)
static void adt7310_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adt7310_data *drv_data = p1;

	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		if (drv_data->th_handler != NULL) {
			drv_data->th_handler(drv_data->dev, drv_data->th_trigger);
		}
	}
}

#elif defined(CONFIG_ADT7310_TRIGGER_GLOBAL_THREAD)
static void adt7310_work_cb(struct k_work *work)
{
	struct adt7310_data *drv_data = CONTAINER_OF(work, struct adt7310_data, work);

	if (drv_data->th_handler != NULL) {
		drv_data->th_handler(drv_data->dev, drv_data->th_trigger);
	}
}
#endif

int adt7310_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct adt7310_data *drv_data = dev->data;
	const struct adt7310_dev_config *cfg = dev->config;

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	/* disable the interrupt but ignore error if it's first */
	/* time this function is called */
	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

	drv_data->th_handler = handler;

	if (handler != NULL) {
		int value, ret;
		gpio_flags_t flags = GPIO_INT_EDGE_TO_ACTIVE;

		drv_data->th_trigger = trig;

		ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, flags);
		if (ret) {
			drv_data->th_handler = NULL;
			return ret;
		}

		value = gpio_pin_get_dt(&cfg->int_gpio);
		if (value > 0) {
#if defined(CONFIG_ADT7310_TRIGGER_OWN_THREAD)
			k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADT7310_TRIGGER_GLOBAL_THREAD)
			k_work_submit(&drv_data->work);
#endif
		}
	}

	return 0;
}

int adt7310_init_interrupt(const struct device *dev)
{
	struct adt7310_data *drv_data = dev->data;
	const struct adt7310_dev_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name, cfg->int_gpio.port->name);
		return -ENODEV;
	}

	gpio_init_callback(&drv_data->gpio_cb, adt7310_gpio_callback, BIT(cfg->int_gpio.pin));

	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT | GPIO_ACTIVE_LOW);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_add_callback(cfg->int_gpio.port, &drv_data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	drv_data->dev = dev;
#if defined(CONFIG_ADT7310_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, 1);
	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ADT7310_THREAD_STACK_SIZE,
			adt7310_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_ADT7310_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&drv_data->thread, dev->name);
#elif defined(CONFIG_ADT7310_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = adt7310_work_cb;
#endif

	return 0;
}
