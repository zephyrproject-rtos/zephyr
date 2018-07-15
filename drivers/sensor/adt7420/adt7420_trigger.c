/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <gpio.h>
#include <i2c.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>
#include "adt7420.h"

static void adt7420_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct adt7420_data *drv_data = dev->driver_data;
	const struct adt7420_dev_config *cfg = dev->config->config_info;
	u8_t status;

	/* Clear the status */
	if (i2c_reg_read_byte(drv_data->i2c, cfg->i2c_addr,
			      ADT7420_REG_STATUS, &status) < 0) {
		return;
	}

	if (drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, &drv_data->th_trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, cfg->int_gpio);
}

static void adt7420_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	struct adt7420_data *drv_data =
		CONTAINER_OF(cb, struct adt7420_data, gpio_cb);
	const struct adt7420_dev_config *cfg = dev->config->config_info;

	gpio_pin_disable_callback(dev, cfg->int_gpio);

#if defined(CONFIG_ADT7420_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

#if defined(CONFIG_ADT7420_TRIGGER_OWN_THREAD)
static void adt7420_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct adt7420_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		adt7420_thread_cb(dev);
	}
}

#elif defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
static void adt7420_work_cb(struct k_work *work)
{
	struct adt7420_data *drv_data =
		CONTAINER_OF(work, struct adt7420_data, work);
	adt7420_thread_cb(drv_data->dev);
}
#endif

int adt7420_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct adt7420_data *drv_data = dev->driver_data;
	const struct adt7420_dev_config *cfg = dev->config->config_info;

	gpio_pin_disable_callback(drv_data->gpio, cfg->int_gpio);

	if (trig->type == SENSOR_TRIG_THRESHOLD) {
		drv_data->th_handler = handler;
		drv_data->th_trigger = *trig;
	} else {
		SYS_LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	gpio_pin_enable_callback(drv_data->gpio, cfg->int_gpio);

	return 0;
}

int adt7420_init_interrupt(struct device *dev)
{
	struct adt7420_data *drv_data = dev->driver_data;
	const struct adt7420_dev_config *cfg = dev->config->config_info;

	drv_data->gpio = device_get_binding(cfg->gpio_port);
	if (drv_data->gpio == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
		    cfg->gpio_port);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, cfg->int_gpio,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   adt7420_gpio_callback,
			   BIT(cfg->int_gpio));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

#if defined(CONFIG_ADT7420_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ADT7420_THREAD_STACK_SIZE,
			(k_thread_entry_t)adt7420_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_ADT7420_THREAD_PRIORITY),
			0, 0);
#elif defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = adt7420_work_cb;
	drv_data->dev = dev;
#endif

	return 0;
}
