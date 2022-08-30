/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT honeywell_hmc5883l

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "hmc5883l.h"

LOG_MODULE_DECLARE(HMC5883L, CONFIG_SENSOR_LOG_LEVEL);

int hmc5883l_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct hmc5883l_data *drv_data = dev->data;
	const struct hmc5883l_config *config = dev->config;

	if (!config->int_gpio.port) {
		return -ENOTSUP;
	}

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = *trig;

	gpio_pin_interrupt_configure_dt(&config->int_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

static void hmc5883l_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct hmc5883l_data *drv_data =
		CONTAINER_OF(cb, struct hmc5883l_data, gpio_cb);
	const struct hmc5883l_config *config = drv_data->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_HMC5883L_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_HMC5883L_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void hmc5883l_thread_cb(const struct device *dev)
{
	struct hmc5883l_data *drv_data = dev->data;
	const struct hmc5883l_config *config = dev->config;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	gpio_pin_interrupt_configure_dt(&config->int_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_HMC5883L_TRIGGER_OWN_THREAD
static void hmc5883l_thread(struct hmc5883l_data *drv_data)
{
	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		hmc5883l_thread_cb(drv_data->dev);
	}
}
#endif

#ifdef CONFIG_HMC5883L_TRIGGER_GLOBAL_THREAD
static void hmc5883l_work_cb(struct k_work *work)
{
	struct hmc5883l_data *drv_data =
		CONTAINER_OF(work, struct hmc5883l_data, work);

	hmc5883l_thread_cb(drv_data->dev);
}
#endif

int hmc5883l_init_interrupt(const struct device *dev)
{
	struct hmc5883l_data *drv_data = dev->data;
	const struct hmc5883l_config *config = dev->config;

	if (!device_is_ready(config->int_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);

	gpio_init_callback(&drv_data->gpio_cb, hmc5883l_gpio_callback,
			   BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Failed to set gpio callback.");
		return -EIO;
	}

	drv_data->dev = dev;

#if defined(CONFIG_HMC5883L_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_HMC5883L_THREAD_STACK_SIZE,
			(k_thread_entry_t)hmc5883l_thread,
			drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_HMC5883L_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_HMC5883L_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = hmc5883l_work_cb;
#endif

	gpio_pin_interrupt_configure_dt(&config->int_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
