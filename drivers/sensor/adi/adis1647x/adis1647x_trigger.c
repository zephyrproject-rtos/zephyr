/*
 * Copyright (c) 2026 Analog Devices Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adis1647x

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "adis1647x.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(adis1647x, CONFIG_SENSOR_LOG_LEVEL);

#if defined(CONFIG_ADIS1647X_TRIGGER_OWN_THREAD) || defined(CONFIG_ADIS1647X_TRIGGER_GLOBAL_THREAD)
static void adis1647x_thread_cb(const struct device *dev)
{
	const struct adis1647x_config *cfg = dev->config;
	struct adis1647x_data *drv_data = dev->data;

#ifdef CONFIG_ADIS1647X_STREAM
	if (drv_data->sqe != NULL) {
		adis1647x_stream_irq_handler(dev); /* re-enables interrupt internally */
		return;
	}
#endif

	if (drv_data->drdy_handler != NULL) {
		drv_data->drdy_handler(dev, drv_data->drdy_trigger);
	}

	if (gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE) < 0) {
		LOG_ERR("Failed to re-enable interrupt");
	}
}
#endif /* CONFIG_ADIS1647X_TRIGGER_OWN_THREAD || CONFIG_ADIS1647X_TRIGGER_GLOBAL_THREAD */

#if defined(CONFIG_ADIS1647X_TRIGGER_OWN_THREAD)
static void adis1647x_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adis1647x_data *drv_data = p1;

	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		adis1647x_thread_cb(drv_data->dev);
	}
}

#elif defined(CONFIG_ADIS1647X_TRIGGER_GLOBAL_THREAD)
static void adis1647x_work_cb(struct k_work *work)
{
	struct adis1647x_data *drv_data = CONTAINER_OF(work, struct adis1647x_data, work);

	adis1647x_thread_cb(drv_data->dev);
}
#endif

static void adis1647x_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				    uint32_t pins)
{
	struct adis1647x_data *drv_data = CONTAINER_OF(cb, struct adis1647x_data, gpio_cb);
	const struct adis1647x_config *cfg = drv_data->dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_DISABLE);

#if defined(CONFIG_ADIS1647X_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADIS1647X_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

int adis1647x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	const struct adis1647x_config *cfg = dev->config;
	struct adis1647x_data *drv_data = dev->data;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_DISABLE);
	if (ret < 0) {
		return ret;
	}

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		LOG_ERR("Unsupported trigger type %d", trig->type);
		return -ENOTSUP;
	}

	drv_data->drdy_handler = handler;
	drv_data->drdy_trigger = trig;

	if (!handler) {
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int adis1647x_init_interrupt(const struct device *dev)
{
	const struct adis1647x_config *cfg = dev->config;
	struct adis1647x_data *drv_data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&cfg->interrupt)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->interrupt.port);
		return -EINVAL;
	}

	ret = gpio_pin_configure_dt(&cfg->interrupt, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&drv_data->gpio_cb, adis1647x_gpio_callback, BIT(cfg->interrupt.pin));

	ret = gpio_add_callback(cfg->interrupt.port, &drv_data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set gpio callback!");
		return ret;
	}

	drv_data->dev = dev;

#if defined(CONFIG_ADIS1647X_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ADIS1647X_THREAD_STACK_SIZE, adis1647x_thread, drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ADIS1647X_THREAD_PRIORITY), 0, K_NO_WAIT);

	k_thread_name_set(&drv_data->thread, dev->name);
#elif defined(CONFIG_ADIS1647X_TRIGGER_GLOBAL_THREAD)
	k_work_init(&drv_data->work, adis1647x_work_cb);
#endif

	return 0;
}
