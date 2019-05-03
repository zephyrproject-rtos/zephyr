/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <i2c.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>

#include "vl53l1x.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(VL53L1X);

int vl53l1x_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct vl53l1x_data *drv_data = dev->driver_data;

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	gpio_pin_disable_callback(drv_data->gpio,
				  DT_ST_VL53L1X_0_DRDY_GPIOS_PIN);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = *trig;

	gpio_pin_enable_callback(drv_data->gpio,
				 DT_ST_VL53L1X_0_DRDY_GPIOS_PIN);

	return 0;
}

static void vl53l1x_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	struct vl53l1x_data *drv_data =
		CONTAINER_OF(cb, struct vl53l1x_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, DT_ST_VL53L1X_0_DRDY_GPIOS_PIN);

#if defined(CONFIG_VL53L1X_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_VL53L1X_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void vl53l1x_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct vl53l1x_data *drv_data = dev->driver_data;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, DT_ST_VL53L1X_0_DRDY_GPIOS_PIN);
}

#ifdef CONFIG_VL53L1X_TRIGGER_OWN_THREAD
static void vl53l1x_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct vl53l1x_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		vl53l1x_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_VL53L1X_TRIGGER_GLOBAL_THREAD
static void vl53l1x_work_cb(struct k_work *work)
{
	struct vl53l1x_data *drv_data =
		CONTAINER_OF(work, struct vl53l1x_data, work);

	vl53l1x_thread_cb(drv_data->dev);
}
#endif

int vl53l1x_init_interrupt(struct device *dev)
{
	struct vl53l1x_data *drv_data = dev->driver_data;

	/* setup data ready gpio interrupt */
	drv_data->gpio =
		device_get_binding(DT_ST_VL53L1X_0_DRDY_GPIOS_CONTROLLER);
	if (drv_data->gpio == NULL) {
		LOG_ERR("Cannot get pointer to %s device.",
			    DT_ST_VL53L1X_0_DRDY_GPIOS_CONTROLLER);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, DT_ST_VL53L1X_0_DRDY_GPIOS_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   vl53l1x_gpio_callback,
			   BIT(DT_ST_VL53L1X_0_DRDY_GPIOS_PIN));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback.");
		return -EIO;
	}

#if defined(CONFIG_VL53L1X_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_VL53L1X_THREAD_STACK_SIZE,
			(k_thread_entry_t)vl53l1x_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_VL53L1X_THREAD_PRIORITY),
			0, 0);
#elif defined(CONFIG_VL53L1X_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = vl53l1x_work_cb;
	drv_data->dev = dev;
#endif

	return 0;
}
