/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp007

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include "tmp007.h"

extern struct tmp007_data tmp007_driver;

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(TMP007, CONFIG_SENSOR_LOG_LEVEL);

static inline void setup_int(const struct device *dev,
			     bool enable)
{
	const struct tmp007_config *cfg = dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
				     enable
				     ? GPIO_INT_LEVEL_ACTIVE
				     : GPIO_INT_DISABLE);
}

int tmp007_attr_set(const struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	const struct tmp007_config *cfg = dev->config;
	int64_t value;
	uint8_t reg;

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

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

	if (tmp007_reg_write(&cfg->i2c, reg, value) < 0) {
		LOG_DBG("Failed to set attribute!");
		return -EIO;
	}

	return 0;
}

static void tmp007_gpio_callback(const struct device *dev,
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

static void tmp007_thread_cb(const struct device *dev)
{
	struct tmp007_data *drv_data = dev->data;
	const struct tmp007_config *cfg = dev->config;
	uint16_t status;

	if (tmp007_reg_read(&cfg->i2c, TMP007_REG_STATUS, &status) < 0) {
		return;
	}

	if (status & TMP007_DATA_READY_INT_BIT &&
	    drv_data->drdy_handler != NULL) {
		drv_data->drdy_handler(dev, drv_data->drdy_trigger);
	}

	if (status & TMP007_TOBJ_TH_INT_BITS &&
	    drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, drv_data->th_trigger);
	}

	setup_int(dev, true);
}

#ifdef CONFIG_TMP007_TRIGGER_OWN_THREAD
static void tmp007_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct tmp007_data *drv_data = p1;

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		tmp007_thread_cb(drv_data->dev);
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

int tmp007_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct tmp007_data *drv_data = dev->data;
	const struct tmp007_config *cfg = dev->config;

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	setup_int(dev, false);

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = trig;
	} else if (trig->type == SENSOR_TRIG_THRESHOLD) {
		drv_data->th_handler = handler;
		drv_data->th_trigger = trig;
	}

	setup_int(dev, true);

	return 0;
}

int tmp007_init_interrupt(const struct device *dev)
{
	struct tmp007_data *drv_data = dev->data;
	const struct tmp007_config *cfg = dev->config;

	if (tmp007_reg_update(&cfg->i2c, TMP007_REG_CONFIG,
			      TMP007_ALERT_EN_BIT, TMP007_ALERT_EN_BIT) < 0) {
		LOG_DBG("Failed to enable interrupt pin!");
		return -EIO;
	}

	drv_data->dev = dev;

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
				cfg->int_gpio.port->name);
		return -ENODEV;
	}

	gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INT_LEVEL_ACTIVE);

	gpio_init_callback(&drv_data->gpio_cb,
			   tmp007_gpio_callback,
			   BIT(cfg->int_gpio.pin));

	if (gpio_add_callback(cfg->int_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

#if defined(CONFIG_TMP007_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_TMP007_THREAD_STACK_SIZE,
			tmp007_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_TMP007_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_TMP007_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = tmp007_work_cb;
#endif

	return 0;
}
