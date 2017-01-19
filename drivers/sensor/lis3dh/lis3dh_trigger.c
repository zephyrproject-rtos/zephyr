/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <i2c.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>

#include "lis3dh.h"

int lis3dh_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct lis3dh_data *drv_data = dev->driver_data;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	gpio_pin_disable_callback(drv_data->gpio, CONFIG_LIS3DH_GPIO_PIN_NUM);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}
	drv_data->data_ready_trigger = *trig;

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_LIS3DH_GPIO_PIN_NUM);

	return 0;
}

static void lis3dh_gpio_callback(struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct lis3dh_data *drv_data =
		CONTAINER_OF(cb, struct lis3dh_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, CONFIG_LIS3DH_GPIO_PIN_NUM);

#if defined(CONFIG_LIS3DH_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_LIS3DH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void lis3dh_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct lis3dh_data *drv_data = dev->driver_data;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_LIS3DH_GPIO_PIN_NUM);
}

#ifdef CONFIG_LIS3DH_TRIGGER_OWN_THREAD
static void lis3dh_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct lis3dh_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		lis3dh_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_LIS3DH_TRIGGER_GLOBAL_THREAD
static void lis3dh_work_cb(struct k_work *work)
{
	struct lis3dh_data *drv_data =
		CONTAINER_OF(work, struct lis3dh_data, work);

	lis3dh_thread_cb(drv_data->dev);
}
#endif

int lis3dh_init_interrupt(struct device *dev)
{
	struct lis3dh_data *drv_data = dev->driver_data;

	/* setup data ready gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_LIS3DH_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		SYS_LOG_DBG("Cannot get pointer to %s device",
			    CONFIG_LIS3DH_GPIO_DEV_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_LIS3DH_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   lis3dh_gpio_callback,
			   BIT(CONFIG_LIS3DH_GPIO_PIN_NUM));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* clear data ready interrupt line by reading sample data */
	if (lis3dh_sample_fetch(dev, SENSOR_CHAN_ALL) < 0) {
		SYS_LOG_DBG("Could not clear data ready interrupt line.");
		return -EIO;
	}

	/* enable data ready interrupt on INT1 line */
	if (i2c_reg_write_byte(drv_data->i2c, LIS3DH_I2C_ADDRESS,
			       LIS3DH_REG_CTRL3, LIS3DH_EN_DRDY1_INT1) < 0) {
		SYS_LOG_DBG("Failed to enable data ready interrupt.");
		return -EIO;
	}

#if defined(CONFIG_LIS3DH_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_spawn(drv_data->thread_stack, CONFIG_LIS3DH_THREAD_STACK_SIZE,
		    (k_thread_entry_t)lis3dh_thread, POINTER_TO_INT(dev),
		    0, NULL, K_PRIO_COOP(CONFIG_LIS3DH_THREAD_PRIORITY), 0, 0);
#elif defined(CONFIG_LIS3DH_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = lis3dh_work_cb;
	drv_data->dev = dev;
#endif

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_LIS3DH_GPIO_PIN_NUM);

	return 0;
}
