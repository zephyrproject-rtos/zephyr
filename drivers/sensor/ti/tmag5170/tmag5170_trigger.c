/*
 * Copyright (c) 2023 Michal Morsisko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmag5170

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#include "tmag5170.h"

LOG_MODULE_DECLARE(TMAG5170, CONFIG_SENSOR_LOG_LEVEL);

static void tmag5170_handle_interrupts(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct tmag5170_data *data = dev->data;

	if (data->handler_drdy) {
		data->handler_drdy(dev, data->trigger_drdy);
	}
}

#if defined(CONFIG_TMAG5170_TRIGGER_OWN_THREAD)
static void tmag5170_thread_main(void *arg1, void *unused1, void *unused2)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	const struct device *dev = (const struct device *)arg1;
	struct tmag5170_data *data = dev->data;

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		tmag5170_handle_interrupts(dev);
	}
}
#endif

#if defined(CONFIG_TMAG5170_TRIGGER_GLOBAL_THREAD)
static void tmag5170_work_handler(struct k_work *work)
{
	struct tmag5170_data *data = CONTAINER_OF(work,
						  struct tmag5170_data,
						  work);

	tmag5170_handle_interrupts(data->dev);
}
#endif

static void tmag5170_gpio_callback(const struct device *port,
				   struct gpio_callback *cb,
				   uint32_t pin)
{
	struct tmag5170_data *data = CONTAINER_OF(cb,
						  struct tmag5170_data,
						  gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

#if defined(CONFIG_TMAG5170_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_TMAG5170_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#elif defined(CONFIG_TMAG5170_TRIGGER_DIRECT)
	tmag5170_handle_interrupts(data->dev);
#endif
}

int tmag5170_trigger_set(
	const struct device *dev,
	const struct sensor_trigger *trig,
	sensor_trigger_handler_t handler)
{
	struct tmag5170_data *data = dev->data;

#if defined(CONFIG_PM_DEVICE)
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return -EBUSY;
	}
#endif

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	data->trigger_drdy = trig;
	data->handler_drdy = handler;

	return 0;
}

int tmag5170_trigger_init(const struct device *dev)
{
	struct tmag5170_data *data = dev->data;
	const struct tmag5170_dev_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->int_gpio.port)) {
		LOG_ERR("%s: device %s is not ready", dev->name, cfg->int_gpio.port->name);
		return -ENODEV;
	}

	data->dev = dev;

#if defined(CONFIG_TMAG5170_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, 1);
	k_thread_create(
		&data->thread,
		data->thread_stack,
		CONFIG_TMAG5170_THREAD_STACK_SIZE,
		tmag5170_thread_main,
		(void *)dev,
		NULL,
		NULL,
		K_PRIO_COOP(CONFIG_TMAG5170_THREAD_PRIORITY),
		0,
		K_NO_WAIT);
#elif defined(CONFIG_TMAG5170_TRIGGER_GLOBAL_THREAD)
	data->work.handler = tmag5170_work_handler;
#endif
	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);

	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, tmag5170_gpio_callback, BIT(cfg->int_gpio.pin));

	ret = gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		return ret;
	}

	return ret;
}
