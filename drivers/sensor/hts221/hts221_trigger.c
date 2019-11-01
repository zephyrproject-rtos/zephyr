/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/i2c.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include "hts221.h"

LOG_MODULE_DECLARE(HTS221, CONFIG_SENSOR_LOG_LEVEL);

int hts221_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct hts221_data *drv_data = dev->driver_data;

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	gpio_pin_disable_callback(drv_data->gpio,
				  DT_INST_0_ST_HTS221_DRDY_GPIOS_PIN);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = *trig;

	gpio_pin_enable_callback(drv_data->gpio,
				 DT_INST_0_ST_HTS221_DRDY_GPIOS_PIN);

	return 0;
}

static void hts221_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	struct hts221_data *drv_data =
		CONTAINER_OF(cb, struct hts221_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, DT_INST_0_ST_HTS221_DRDY_GPIOS_PIN);

#if defined(CONFIG_HTS221_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_HTS221_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void hts221_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct hts221_data *drv_data = dev->driver_data;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, DT_INST_0_ST_HTS221_DRDY_GPIOS_PIN);
}

#ifdef CONFIG_HTS221_TRIGGER_OWN_THREAD
static void hts221_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct hts221_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		hts221_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_HTS221_TRIGGER_GLOBAL_THREAD
static void hts221_work_cb(struct k_work *work)
{
	struct hts221_data *drv_data =
		CONTAINER_OF(work, struct hts221_data, work);

	hts221_thread_cb(drv_data->dev);
}
#endif

int hts221_init_interrupt(struct device *dev)
{
	struct hts221_data *drv_data = dev->driver_data;

	/* setup data ready gpio interrupt */
	drv_data->gpio =
		device_get_binding(DT_INST_0_ST_HTS221_DRDY_GPIOS_CONTROLLER);
	if (drv_data->gpio == NULL) {
		LOG_ERR("Cannot get pointer to %s device.",
			    DT_INST_0_ST_HTS221_DRDY_GPIOS_CONTROLLER);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, DT_INST_0_ST_HTS221_DRDY_GPIOS_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   hts221_gpio_callback,
			   BIT(DT_INST_0_ST_HTS221_DRDY_GPIOS_PIN));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback.");
		return -EIO;
	}

	/* enable data-ready interrupt */
	if (i2c_reg_write_byte(drv_data->i2c, DT_INST_0_ST_HTS221_BASE_ADDRESS,
			       HTS221_REG_CTRL3, HTS221_DRDY_EN) < 0) {
		LOG_ERR("Could not enable data-ready interrupt.");
		return -EIO;
	}

#if defined(CONFIG_HTS221_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_HTS221_THREAD_STACK_SIZE,
			(k_thread_entry_t)hts221_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_HTS221_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_HTS221_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = hts221_work_cb;
	drv_data->dev = dev;
#endif

	gpio_pin_enable_callback(drv_data->gpio, DT_INST_0_ST_HTS221_DRDY_GPIOS_PIN);

	return 0;
}
