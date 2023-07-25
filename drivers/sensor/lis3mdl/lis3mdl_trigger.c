/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis3mdl_magn

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "lis3mdl.h"

LOG_MODULE_DECLARE(LIS3MDL, CONFIG_SENSOR_LOG_LEVEL);

int lis3mdl_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct lis3mdl_data *drv_data = dev->data;
	const struct lis3mdl_config *config = dev->config;
	int16_t buf[3];
	int ret;

	if (!config->irq_gpio.port) {
		return -ENOTSUP;
	}

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	/* dummy read: re-trigger interrupt */
	ret = i2c_burst_read_dt(&config->i2c, LIS3MDL_REG_SAMPLE_START,
				(uint8_t *)buf, 6);
	if (ret != 0) {
		return ret;
	}

	gpio_pin_interrupt_configure_dt(&config->irq_gpio, GPIO_INT_DISABLE);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = trig;

	gpio_pin_interrupt_configure_dt(&config->irq_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

static void lis3mdl_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct lis3mdl_data *drv_data =
		CONTAINER_OF(cb, struct lis3mdl_data, gpio_cb);
	const struct lis3mdl_config *config = drv_data->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&config->irq_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_LIS3MDL_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_LIS3MDL_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void lis3mdl_thread_cb(const struct device *dev)
{
	struct lis3mdl_data *drv_data = dev->data;
	const struct lis3mdl_config *config = dev->config;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     drv_data->data_ready_trigger);
	}

	gpio_pin_interrupt_configure_dt(&config->irq_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_LIS3MDL_TRIGGER_OWN_THREAD
static void lis3mdl_thread(struct lis3mdl_data *drv_data)
{
	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		lis3mdl_thread_cb(drv_data->dev);
	}
}
#endif

#ifdef CONFIG_LIS3MDL_TRIGGER_GLOBAL_THREAD
static void lis3mdl_work_cb(struct k_work *work)
{
	struct lis3mdl_data *drv_data =
		CONTAINER_OF(work, struct lis3mdl_data, work);

	lis3mdl_thread_cb(drv_data->dev);
}
#endif

int lis3mdl_init_interrupt(const struct device *dev)
{
	struct lis3mdl_data *drv_data = dev->data;
	const struct lis3mdl_config *config = dev->config;

	if (!device_is_ready(config->irq_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->irq_gpio, GPIO_INPUT);

	gpio_init_callback(&drv_data->gpio_cb,
			   lis3mdl_gpio_callback,
			   BIT(config->irq_gpio.pin));

	if (gpio_add_callback(config->irq_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback.");
		return -EIO;
	}

	/* clear data ready interrupt line by reading sample data */
	if (lis3mdl_sample_fetch(dev, SENSOR_CHAN_ALL) < 0) {
		LOG_DBG("Could not clear data ready interrupt line.");
		return -EIO;
	}

	drv_data->dev = dev;

#if defined(CONFIG_LIS3MDL_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_LIS3MDL_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis3mdl_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_LIS3MDL_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_LIS3MDL_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = lis3mdl_work_cb;
#endif

	gpio_pin_interrupt_configure_dt(&config->irq_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
