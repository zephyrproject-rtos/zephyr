/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp007

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>

#include "tmp007.h"

extern struct tmp007_data tmp007_driver;

#include <logging/log.h>
LOG_MODULE_DECLARE(TMP007, CONFIG_SENSOR_LOG_LEVEL);

static inline void setup_int(struct device *dev,
			     bool enable)
{
	struct tmp007_data *data = dev->data;

	gpio_pin_interrupt_configure(data->gpio,
				     DT_INST_GPIO_PIN(0, int_gpios),
				     enable
				     ? GPIO_INT_LEVEL_ACTIVE
				     : GPIO_INT_DISABLE);
}

int tmp007_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	struct tmp007_data *drv_data = dev->data;
	int64_t value;
	uint8_t reg;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		reg = TMP007_REG_TOBJ_TH_HIGH;
	} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
		reg = TMP007_REG_TOBJ_TH_LOW;
	} else {
		return -ENOTSUP;
	}

	value = (int64_t)val->val1 * 1000000 + val->val2;
	value = (value / TMP007_TEMP_TH_SCALE) << 6;

	if (tmp007_reg_write(drv_data, reg, value) < 0) {
		LOG_DBG("Failed to set attribute!");
		return -EIO;
	}

	return 0;
}

static void tmp007_gpio_callback(struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct tmp007_data *drv_data =
		CONTAINER_OF(cb, struct tmp007_data, gpio_cb);

	setup_int(drv_data->dev, false);

#if defined(CONFIG_TMP007_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_TMP007_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void tmp007_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct tmp007_data *drv_data = dev->data;
	uint16_t status;

	if (tmp007_reg_read(drv_data, TMP007_REG_STATUS, &status) < 0) {
		return;
	}

	if (status & TMP007_DATA_READY_INT_BIT &&
	    drv_data->drdy_handler != NULL) {
		drv_data->drdy_handler(dev, &drv_data->drdy_trigger);
	}

	if (status & TMP007_TOBJ_TH_INT_BITS &&
	    drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, &drv_data->th_trigger);
	}

	setup_int(dev, true);
}

#ifdef CONFIG_TMP007_TRIGGER_OWN_THREAD
static void tmp007_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct tmp007_data *drv_data = dev->data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		tmp007_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_TMP007_TRIGGER_GLOBAL_THREAD
static void tmp007_work_cb(struct k_work *work)
{
	struct tmp007_data *drv_data =
		CONTAINER_OF(work, struct tmp007_data, work);

	tmp007_thread_cb(drv_data->dev);
}
#endif

int tmp007_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct tmp007_data *drv_data = dev->data;

	setup_int(dev, false);

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = *trig;
	} else if (trig->type == SENSOR_TRIG_THRESHOLD) {
		drv_data->th_handler = handler;
		drv_data->th_trigger = *trig;
	}

	setup_int(dev, true);

	return 0;
}

int tmp007_init_interrupt(struct device *dev)
{
	struct tmp007_data *drv_data = dev->data;

	if (tmp007_reg_update(drv_data, TMP007_REG_CONFIG,
			      TMP007_ALERT_EN_BIT, TMP007_ALERT_EN_BIT) < 0) {
		LOG_DBG("Failed to enable interrupt pin!");
		return -EIO;
	}

	drv_data->dev = dev;

	/* setup gpio interrupt */
	drv_data->gpio = device_get_binding(DT_INST_GPIO_LABEL(0, int_gpios));
	if (drv_data->gpio == NULL) {
		LOG_DBG("Failed to get pointer to %s device!",
		    DT_INST_GPIO_LABEL(0, int_gpios));
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, DT_INST_GPIO_PIN(0, int_gpios),
			   DT_INST_GPIO_FLAGS(0, int_gpios)
			   | GPIO_INT_LEVEL_ACTIVE);

	gpio_init_callback(&drv_data->gpio_cb,
			   tmp007_gpio_callback,
			   BIT(DT_INST_GPIO_PIN(0, int_gpios)));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

#if defined(CONFIG_TMP007_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_TMP007_THREAD_STACK_SIZE,
			(k_thread_entry_t)tmp007_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_TMP007_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_TMP007_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = tmp007_work_cb;
#endif

	return 0;
}
