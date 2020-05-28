/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis3mdl_magn

#include <device.h>
#include <drivers/i2c.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include "lis3mdl.h"

LOG_MODULE_DECLARE(LIS3MDL, CONFIG_SENSOR_LOG_LEVEL);

int lis3mdl_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct lis3mdl_data *drv_data = dev->data;
	int16_t buf[3];
	int ret;

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	/* dummy read: re-trigger interrupt */
	ret = i2c_burst_read(drv_data->i2c, DT_INST_REG_ADDR(0),
			     LIS3MDL_REG_SAMPLE_START, (uint8_t *)buf, 6);
	if (ret != 0) {
		return ret;
	}

	gpio_pin_interrupt_configure(drv_data->gpio,
			DT_INST_GPIO_PIN(0, irq_gpios),
			GPIO_INT_DISABLE);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = *trig;

	gpio_pin_interrupt_configure(drv_data->gpio,
			DT_INST_GPIO_PIN(0, irq_gpios),
			GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

static void lis3mdl_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct lis3mdl_data *drv_data =
		CONTAINER_OF(cb, struct lis3mdl_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure(dev,
				     DT_INST_GPIO_PIN(0, irq_gpios),
				     GPIO_INT_DISABLE);

#if defined(CONFIG_LIS3MDL_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_LIS3MDL_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void lis3mdl_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct lis3mdl_data *drv_data = dev->data;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	gpio_pin_interrupt_configure(drv_data->gpio,
			DT_INST_GPIO_PIN(0, irq_gpios),
			GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_LIS3MDL_TRIGGER_OWN_THREAD
static void lis3mdl_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct lis3mdl_data *drv_data = dev->data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		lis3mdl_thread_cb(dev);
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

int lis3mdl_init_interrupt(struct device *dev)
{
	struct lis3mdl_data *drv_data = dev->data;

	/* setup data ready gpio interrupt */
	drv_data->gpio =
		device_get_binding(DT_INST_GPIO_LABEL(0, irq_gpios));
	if (drv_data->gpio == NULL) {
		LOG_DBG("Cannot get pointer to %s device.",
			    DT_INST_GPIO_LABEL(0, irq_gpios));
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio,
			   DT_INST_GPIO_PIN(0, irq_gpios),
			   GPIO_INPUT |
			   DT_INST_GPIO_FLAGS(0, irq_gpios));

	gpio_init_callback(&drv_data->gpio_cb,
			   lis3mdl_gpio_callback,
			   BIT(DT_INST_GPIO_PIN(0, irq_gpios)));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback.");
		return -EIO;
	}

	/* clear data ready interrupt line by reading sample data */
	if (lis3mdl_sample_fetch(dev, SENSOR_CHAN_ALL) < 0) {
		LOG_DBG("Could not clear data ready interrupt line.");
		return -EIO;
	}

#if defined(CONFIG_LIS3MDL_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_LIS3MDL_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis3mdl_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_LIS3MDL_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_LIS3MDL_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = lis3mdl_work_cb;
	drv_data->dev = dev;
#endif

	gpio_pin_interrupt_configure(drv_data->gpio,
			DT_INST_GPIO_PIN(0, irq_gpios),
			GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
