/*
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/sensor.h>
#include "ccs811.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(CCS811);

int ccs811_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *thr)
{
	struct ccs811_data *drv_data = dev->driver_data;
	int rc;

	if (chan != SENSOR_CHAN_CO2) {
		rc = -ENOTSUP;
	} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
		rc = -EINVAL;
		if ((thr->val1 >= CCS811_CO2_MIN_PPM)
		    && (thr->val1 <= CCS811_CO2_MAX_PPM)) {
			drv_data->co2_l2m = thr->val1;
			rc = 0;
		}
	} else if (attr == SENSOR_ATTR_UPPER_THRESH) {
		rc = -EINVAL;
		if ((thr->val1 >= CCS811_CO2_MIN_PPM)
		    && (thr->val1 <= CCS811_CO2_MAX_PPM)) {
			drv_data->co2_m2h = thr->val1;
			rc = 0;
		}
	} else {
		rc = -ENOTSUP;
	}
	return rc;
}

static void gpio_callback(struct device *dev,
			  struct gpio_callback *cb,
			  u32_t pins)
{
	struct ccs811_data *drv_data =
		CONTAINER_OF(cb, struct ccs811_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, DT_INST_0_AMS_CCS811_IRQ_GPIOS_PIN);

#if defined(CONFIG_CCS811_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_CCS811_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#else
#error Unhandled trigger configuration
#endif
}

static void thread_cb(void *arg)
{
	struct device *dev = arg;
	struct ccs811_data *drv_data = dev->driver_data;

	if (drv_data->handler != NULL) {
		drv_data->handler(dev, &drv_data->trigger);
	}

	gpio_pin_enable_callback(drv_data->int_gpio, DT_INST_0_AMS_CCS811_IRQ_GPIOS_PIN);
}

#ifdef CONFIG_CCS811_TRIGGER_OWN_THREAD
static void datardy_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct ccs811_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		thread_cb(dev);
	}
}
#elif defined(CONFIG_CCS811_TRIGGER_GLOBAL_THREAD)
static void work_cb(struct k_work *work)
{
	struct ccs811_data *drv_data =
		CONTAINER_OF(work, struct ccs811_data, work);

	thread_cb(drv_data->dev);
}
#else
#error Unhandled trigger configuration
#endif

int ccs811_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct ccs811_data *drv_data = dev->driver_data;
	u8_t drdy_thresh = CCS811_MODE_THRESH | CCS811_MODE_DATARDY;
	int rc;

	LOG_DBG("CCS811 trigger set");
	gpio_pin_disable_callback(drv_data->int_gpio, DT_INST_0_AMS_CCS811_IRQ_GPIOS_PIN);
	if (trig->type == SENSOR_TRIG_DATA_READY) {
		rc = ccs811_mutate_meas_mode(dev, CCS811_MODE_DATARDY,
					     CCS811_MODE_THRESH);
	} else if (trig->type == SENSOR_TRIG_THRESHOLD) {
		rc = -EINVAL;
		if ((drv_data->co2_l2m >= CCS811_CO2_MIN_PPM)
		    && (drv_data->co2_l2m <= CCS811_CO2_MAX_PPM)
		    && (drv_data->co2_m2h >= CCS811_CO2_MIN_PPM)
		    && (drv_data->co2_m2h <= CCS811_CO2_MAX_PPM)
		    && (drv_data->co2_l2m <= drv_data->co2_m2h)) {
			rc = ccs811_set_thresholds(dev);
		}
		if (rc == 0) {
			rc = ccs811_mutate_meas_mode(dev, drdy_thresh, 0);
		}
	} else {
		rc = -ENOTSUP;
	}

	if (rc == 0) {
		drv_data->handler = handler;
		drv_data->trigger = *trig;
		gpio_pin_enable_callback(drv_data->int_gpio,
					 DT_INST_0_AMS_CCS811_IRQ_GPIOS_PIN);
	} else {
		(void)ccs811_mutate_meas_mode(dev, 0, drdy_thresh);
	}

	return rc;
}

int ccs811_init_interrupt(struct device *dev)
{
	struct ccs811_data *drv_data = dev->driver_data;

#ifndef DT_INST_0_AMS_CCS811_IRQ_GPIOS_PIN
	return -EINVAL;
#endif
	gpio_pin_configure(drv_data->int_gpio, DT_INST_0_AMS_CCS811_IRQ_GPIOS_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL |
			   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP |
			   GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb, gpio_callback,
			   BIT(DT_INST_0_AMS_CCS811_IRQ_GPIOS_PIN));

	if (gpio_add_callback(drv_data->int_gpio, &drv_data->gpio_cb) < 0) {
		LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

#if defined(CONFIG_CCS811_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_CCS811_THREAD_STACK_SIZE,
			(k_thread_entry_t)datardy_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_CCS811_THREAD_PRIORITY),
			0, 0);
#elif defined(CONFIG_CCS811_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = work_cb;
	drv_data->dev = dev;
#else
#error Unhandled trigger configuration
#endif
	return 0;
}
