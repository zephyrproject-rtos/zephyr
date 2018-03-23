/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>

#include "sht3xd.h"

static u16_t sht3xd_temp_processed_to_raw(const struct sensor_value *val)
{
	u64_t uval;

	/* ret = (val + 45) * (2^16 - 1) / 175 */
	uval = (u64_t)(val->val1 + 45) * 1000000 + val->val2;
	return ((uval * 0xFFFF) / 175) / 1000000;
}

static int sht3xd_rh_processed_to_raw(const struct sensor_value *val)
{
	u64_t uval;

	/* ret = val * (2^16 -1) / 100000 */
	uval = (u64_t)val->val1 * 1000000 + val->val2;
	return ((uval * 0xFFFF) / 100000) / 1000000;
}

int sht3xd_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	struct sht3xd_data *drv_data = dev->driver_data;
	u16_t set_cmd, clear_cmd, reg_val, temp, rh;

	if (attr == SENSOR_ATTR_LOWER_THRESH) {
		if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
			drv_data->t_low = sht3xd_temp_processed_to_raw(val);
		} else if (chan == SENSOR_CHAN_HUMIDITY) {
			drv_data->rh_low = sht3xd_rh_processed_to_raw(val);
		} else {
			return -ENOTSUP;
		}

		set_cmd = SHT3XD_CMD_WRITE_TH_LOW_SET;
		clear_cmd = SHT3XD_CMD_WRITE_TH_LOW_CLEAR;
		temp = drv_data->t_low;
		rh = drv_data->rh_low;
	} else if (attr == SENSOR_ATTR_UPPER_THRESH) {
		if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
			drv_data->t_high = sht3xd_temp_processed_to_raw(val);
		} else if (chan == SENSOR_CHAN_HUMIDITY) {
			drv_data->rh_high = sht3xd_rh_processed_to_raw(val);
		} else {
			return -ENOTSUP;
		}

		set_cmd = SHT3XD_CMD_WRITE_TH_HIGH_SET;
		clear_cmd = SHT3XD_CMD_WRITE_TH_HIGH_CLEAR;
		temp = drv_data->t_high;
		rh = drv_data->rh_high;
	} else {
		return -ENOTSUP;
	}

	reg_val = (rh & 0xFE00) | ((temp & 0xFF80) >> 7);

	if (sht3xd_write_reg(drv_data, set_cmd, reg_val) < 0 ||
	    sht3xd_write_reg(drv_data, clear_cmd, reg_val) < 0) {
		SYS_LOG_DBG("Failed to write threshold value!");
		return -EIO;
	}

	return 0;
}

static void sht3xd_gpio_callback(struct device *dev,
				 struct gpio_callback *cb, u32_t pins)
{
	struct sht3xd_data *drv_data =
		CONTAINER_OF(cb, struct sht3xd_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, CONFIG_SHT3XD_GPIO_PIN_NUM);

#if defined(CONFIG_SHT3XD_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_SHT3XD_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void sht3xd_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct sht3xd_data *drv_data = dev->driver_data;

	if (drv_data->handler != NULL) {
		drv_data->handler(dev, &drv_data->trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_SHT3XD_GPIO_PIN_NUM);
}

#ifdef CONFIG_SHT3XD_TRIGGER_OWN_THREAD
static void sht3xd_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct sht3xd_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		sht3xd_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_SHT3XD_TRIGGER_GLOBAL_THREAD
static void sht3xd_work_cb(struct k_work *work)
{
	struct sht3xd_data *drv_data =
		CONTAINER_OF(work, struct sht3xd_data, work);

	sht3xd_thread_cb(drv_data->dev);
}
#endif

int sht3xd_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct sht3xd_data *drv_data = dev->driver_data;

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		return -ENOTSUP;
	}

	gpio_pin_disable_callback(drv_data->gpio, CONFIG_SHT3XD_GPIO_PIN_NUM);
	drv_data->handler = handler;
	drv_data->trigger = *trig;
	gpio_pin_enable_callback(drv_data->gpio, CONFIG_SHT3XD_GPIO_PIN_NUM);

	return 0;
}

int sht3xd_init_interrupt(struct device *dev)
{
	struct sht3xd_data *drv_data = dev->driver_data;

	drv_data->t_low = 0;
	drv_data->rh_low = 0;
	drv_data->t_high = 0xFFFF;
	drv_data->rh_high = 0xFFFF;

	/* set alert thresholds to match reamsurement ranges */
	if (sht3xd_write_reg(drv_data, SHT3XD_CMD_WRITE_TH_HIGH_SET, 0xFFFF)
			     < 0) {
		SYS_LOG_DBG("Failed to write threshold high set value!");
		return -EIO;
	}

	if (sht3xd_write_reg(drv_data, SHT3XD_CMD_WRITE_TH_HIGH_CLEAR,
			     0xFFFF) < 0) {
		SYS_LOG_DBG("Failed to write threshold high clear value!");
		return -EIO;
	}

	if (sht3xd_write_reg(drv_data, SHT3XD_CMD_WRITE_TH_LOW_SET, 0) < 0) {
		SYS_LOG_DBG("Failed to write threshold low set value!");
		return -EIO;
	}

	if (sht3xd_write_reg(drv_data, SHT3XD_CMD_WRITE_TH_LOW_SET, 0) < 0) {
		SYS_LOG_DBG("Failed to write threshold low clear value!");
		return -EIO;
	}

	/* setup gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_SHT3XD_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
		    CONFIG_SHT3XD_GPIO_DEV_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_SHT3XD_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   sht3xd_gpio_callback,
			   BIT(CONFIG_SHT3XD_GPIO_PIN_NUM));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

#if defined(CONFIG_SHT3XD_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_SHT3XD_THREAD_STACK_SIZE,
			(k_thread_entry_t)sht3xd_thread, POINTER_TO_INT(dev),
			0, NULL, K_PRIO_COOP(CONFIG_SHT3XD_THREAD_PRIORITY),
			0, 0);
#elif defined(CONFIG_SHT3XD_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = sht3xd_work_cb;
	drv_data->dev = dev;
#endif

	return 0;
}
