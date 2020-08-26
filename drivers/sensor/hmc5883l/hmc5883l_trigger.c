/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT honeywell_hmc5883l

#include <device.h>
#include <drivers/i2c.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include "hmc5883l.h"

LOG_MODULE_DECLARE(HMC5883L, CONFIG_SENSOR_LOG_LEVEL);

int hmc5883l_trigger_set(struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct hmc5883l_data *drv_data = dev->data;

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	gpio_pin_interrupt_configure(drv_data->gpio,
				     DT_INST_GPIO_PIN(0, int_gpios),
				     GPIO_INT_DISABLE);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = *trig;

	gpio_pin_interrupt_configure(drv_data->gpio,
				     DT_INST_GPIO_PIN(0, int_gpios),
				     GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

static void hmc5883l_gpio_callback(struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct hmc5883l_data *drv_data =
		CONTAINER_OF(cb, struct hmc5883l_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure(dev,
				     DT_INST_GPIO_PIN(0, int_gpios),
				     GPIO_INT_DISABLE);

#if defined(CONFIG_HMC5883L_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_HMC5883L_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void hmc5883l_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct hmc5883l_data *drv_data = dev->data;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	gpio_pin_interrupt_configure(drv_data->gpio,
				     DT_INST_GPIO_PIN(0, int_gpios),
				     GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_HMC5883L_TRIGGER_OWN_THREAD
static void hmc5883l_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct hmc5883l_data *drv_data = dev->data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		hmc5883l_thread_cb(dev);
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

int hmc5883l_init_interrupt(struct device *dev)
{
	struct hmc5883l_data *drv_data = dev->data;

	/* setup data ready gpio interrupt */
	drv_data->gpio = device_get_binding(
		DT_INST_GPIO_LABEL(0, int_gpios));
	if (drv_data->gpio == NULL) {
		LOG_ERR("Failed to get pointer to %s device.",
			DT_INST_GPIO_LABEL(0, int_gpios));
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio,
			   DT_INST_GPIO_PIN(0, int_gpios),
			   GPIO_INPUT |
			   DT_INST_GPIO_FLAGS(0, int_gpios));

	gpio_init_callback(&drv_data->gpio_cb,
			   hmc5883l_gpio_callback,
			   BIT(DT_INST_GPIO_PIN(0, int_gpios)));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Failed to set gpio callback.");
		return -EIO;
	}

#if defined(CONFIG_HMC5883L_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_HMC5883L_THREAD_STACK_SIZE,
			(k_thread_entry_t)hmc5883l_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_HMC5883L_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_HMC5883L_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = hmc5883l_work_cb;
	drv_data->dev = dev;
#endif

	gpio_pin_interrupt_configure(drv_data->gpio,
				     DT_INST_GPIO_PIN(0, int_gpios),
				     GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
