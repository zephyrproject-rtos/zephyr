/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * LIS2MDL mag interrupt configuration and management
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sensor.h>
#include <gpio.h>

#include "lis2mdl.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(LIS2MDL);

static int lis2mdl_enable_int(struct device *dev, int enable)
{
	struct lis2mdl_data *lis2mdl = dev->driver_data;

	/* set interrupt on mag */
	return i2c_reg_update_byte(lis2mdl->i2c, lis2mdl->i2c_addr,
				   LIS2MDL_CFG_REG_C,
				   LIS2MDL_INT_MAG_MASK,
				   enable ? LIS2MDL_INT_MAG : 0);
}

/* link external trigger to event data ready */
int lis2mdl_trigger_set(struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct lis2mdl_data *lis2mdl = dev->driver_data;
	u8_t raw[LIS2MDL_OUT_REG_SIZE];

	if (trig->chan == SENSOR_CHAN_MAGN_XYZ) {
		lis2mdl->handler_drdy = handler;
		if (handler) {
			/* fetch raw data sample: re-trigger lost interrupt */
			if (i2c_burst_read(lis2mdl->i2c, lis2mdl->i2c_addr,
					   LIS2MDL_OUT_REG, raw,
					   sizeof(raw)) < 0) {
				LOG_ERR("Failed to fetch raw data sample.");
				return -EIO;
			}
			return lis2mdl_enable_int(dev, 1);
		} else {
			return lis2mdl_enable_int(dev, 0);
		}
	}

	return -ENOTSUP;
}

/* handle the drdy event: read data and call handler if registered any */
static void lis2mdl_handle_interrupt(void *arg)
{
	struct device *dev = arg;
	struct lis2mdl_data *lis2mdl = dev->driver_data;
	const struct lis2mdl_device_config *const config =
						dev->config->config_info;
	struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
	};

	if (lis2mdl->handler_drdy != NULL) {
		lis2mdl->handler_drdy(dev, &drdy_trigger);
	}

	gpio_pin_enable_callback(lis2mdl->gpio, config->gpio_pin);
}

static void lis2mdl_gpio_callback(struct device *dev,
				    struct gpio_callback *cb, u32_t pins)
{
	struct lis2mdl_data *lis2mdl =
		CONTAINER_OF(cb, struct lis2mdl_data, gpio_cb);
	const struct lis2mdl_device_config *const config =
						dev->config->config_info;

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, config->gpio_pin);

#if defined(CONFIG_LIS2MDL_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2mdl->gpio_sem);
#elif defined(CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2mdl->work);
#endif
}

#ifdef CONFIG_LIS2MDL_TRIGGER_OWN_THREAD
static void lis2mdl_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct lis2mdl_data *lis2mdl = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&lis2mdl->gpio_sem, K_FOREVER);
		lis2mdl_handle_interrupt(dev);
	}
}
#endif

#ifdef CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD
static void lis2mdl_work_cb(struct k_work *work)
{
	struct lis2mdl_data *lis2mdl =
		CONTAINER_OF(work, struct lis2mdl_data, work);

	lis2mdl_handle_interrupt(lis2mdl->dev);
}
#endif

int lis2mdl_init_interrupt(struct device *dev)
{
	struct lis2mdl_data *lis2mdl = dev->driver_data;
	const struct lis2mdl_device_config *const config =
						dev->config->config_info;

	/* setup data ready gpio interrupt */
	lis2mdl->gpio = device_get_binding(config->gpio_name);
	if (lis2mdl->gpio == NULL) {
		LOG_DBG("Cannot get pointer to %s device",
			    config->gpio_name);
		return -EINVAL;
	}

	gpio_pin_configure(lis2mdl->gpio, config->gpio_pin,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&lis2mdl->gpio_cb,
			   lis2mdl_gpio_callback,
			   BIT(config->gpio_pin));

	if (gpio_add_callback(lis2mdl->gpio, &lis2mdl->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

#if defined(CONFIG_LIS2MDL_TRIGGER_OWN_THREAD)
	k_sem_init(&lis2mdl->gpio_sem, 0, UINT_MAX);
	k_thread_create(&lis2mdl->thread, lis2mdl->thread_stack,
			CONFIG_LIS2MDL_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis2mdl_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_LIS2MDL_THREAD_PRIORITY),
			0, 0);
#elif defined(CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD)
	lis2mdl->work.handler = lis2mdl_work_cb;
	lis2mdl->dev = dev;
#endif

	gpio_pin_enable_callback(lis2mdl->gpio, config->gpio_pin);

	return 0;
}
