/*
 * Copyright (c) 2020 Kalyan Sriram <kalyan@coderkalyan.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/gpio.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "hx711.h"

LOG_MODULE_DECLARE(HX711, CONFIG_SENSOR_LOG_LEVEL);

int hx711_trigger_set(const struct device *dev,
		const struct sensor_trigger *trig,
		sensor_trigger_handler_t handler)
{
	struct hx711_data *drv_data = dev->data;
	const struct hx711_config *cfg = dev->config;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	gpio_pin_interrupt_configure(drv_data->dout, cfg->dout_pin,
			GPIO_INT_DISABLE);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = *trig;

	gpio_pin_interrupt_configure(drv_data->dout, cfg->dout_pin,
			GPIO_INT_EDGE_TO_ACTIVE);
	return 0;
}

static void hx711_dout_callback(const struct device *dev,
		struct gpio_callback *cb, uint32_t pins)
{
	struct hx711_data *drv_data =
		CONTAINER_OF(cb, struct hx711_data, dout_cb);
	const struct hx711_config *cfg = drv_data->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure(drv_data->dout, cfg->dout_pin,
			GPIO_INT_DISABLE);

#if defined(CONFIG_HX711_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->dout_sem);
#elif defined(CONFIG_HX711_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void hx711_thread_cb(const struct device *dev)
{
	struct hx711_data *drv_data = dev->data;
	const struct hx711_config *cfg = dev->config;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
				&drv_data->data_ready_trigger);
	}

	gpio_pin_interrupt_configure(drv_data->dout, cfg->dout_pin,
			GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_HX711_TRIGGER_OWN_THREAD
static void hx711_thread(struct hx711_data *drv_data)
{
	while (1) {
		k_sem_take(&drv_data->dout_sem, K_FOREVER);
		hx711_thread_cb(drv_data->dev);
	}
}
#endif

#ifdef CONFIG_HX711_TRIGGER_GLOBAL_THREAD
static void hx711_work_cb(struct k_work *work)
{
	struct hx711_data *drv_data =
		CONTAINER_OF(work, struct hx711_data, work);

	hx711_thread_cb(drv_data->dev);
}
#endif

int hx711_init_interrupt(const struct device *dev)
{
	struct hx711_data *drv_data = dev->data;
	const struct hx711_config *cfg = dev->config;

	/* setup data ready gpio interrupt */
	drv_data->dev = dev;

	gpio_init_callback(&drv_data->dout_cb,
			hx711_dout_callback,
			BIT(cfg->dout_pin));

	if (gpio_add_callback(drv_data->dout, &drv_data->dout_cb) < 0) {
		LOG_ERR("Failed to set gpio callback");
		return -EIO;
	}

#if defined(CONFIG_HX711_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->dout_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_HX711_THREAD_STACK_SIZE,
			(k_thread_entry_t) hx711_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_HX711_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_HX711_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = hx711_work_cb;
#endif

	gpio_pin_interrupt_configure(drv_data->dout, cfg->dout_pin,
			GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
