/* ST Microelectronics LIS2MDL 3-axis magnetometer sensor
 *
 * Copyright (c) 2018-2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2mdl.pdf
 */

#define DT_DRV_COMPAT st_lis2mdl

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include "lis2mdl.h"

LOG_MODULE_DECLARE(LIS2MDL, CONFIG_SENSOR_LOG_LEVEL);

static int lis2mdl_enable_int(const struct device *dev, int enable)
{
	struct lis2mdl_data *lis2mdl = dev->data;

	/* set interrupt on mag */
	return lis2mdl_drdy_on_pin_set(lis2mdl->ctx, enable);
}

/* link external trigger to event data ready */
int lis2mdl_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct lis2mdl_data *lis2mdl = dev->data;
	int16_t raw[3];

	if (trig->chan == SENSOR_CHAN_MAGN_XYZ) {
		lis2mdl->handler_drdy = handler;
		if (handler) {
			/* fetch raw data sample: re-trigger lost interrupt */
			lis2mdl_magnetic_raw_get(lis2mdl->ctx, raw);

			return lis2mdl_enable_int(dev, 1);
		} else {
			return lis2mdl_enable_int(dev, 0);
		}
	}

	return -ENOTSUP;
}

/* handle the drdy event: read data and call handler if registered any */
static void lis2mdl_handle_interrupt(const struct device *dev)
{
	struct lis2mdl_data *lis2mdl = dev->data;
	const struct lis2mdl_config *const config = dev->config;
	struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
	};

	if (lis2mdl->handler_drdy != NULL) {
		lis2mdl->handler_drdy(dev, &drdy_trigger);
	}

	gpio_pin_interrupt_configure(lis2mdl->gpio, config->gpio_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

static void lis2mdl_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct lis2mdl_data *lis2mdl =
		CONTAINER_OF(cb, struct lis2mdl_data, gpio_cb);
	const struct lis2mdl_config *const config = lis2mdl->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure(dev, config->gpio_pin, GPIO_INT_DISABLE);

#if defined(CONFIG_LIS2MDL_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2mdl->gpio_sem);
#elif defined(CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2mdl->work);
#endif
}

#ifdef CONFIG_LIS2MDL_TRIGGER_OWN_THREAD
static void lis2mdl_thread(struct lis2mdl_data *lis2mdl)
{
	while (1) {
		k_sem_take(&lis2mdl->gpio_sem, K_FOREVER);
		lis2mdl_handle_interrupt(lis2mdl->dev);
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

int lis2mdl_init_interrupt(const struct device *dev)
{
	struct lis2mdl_data *lis2mdl = dev->data;
	const struct lis2mdl_config *const config = dev->config;

	/* setup data ready gpio interrupt */
	lis2mdl->gpio = device_get_binding(config->gpio_name);
	if (lis2mdl->gpio == NULL) {
		LOG_DBG("Cannot get pointer to %s device",
			    config->gpio_name);
		return -EINVAL;
	}

#if defined(CONFIG_LIS2MDL_TRIGGER_OWN_THREAD)
	k_sem_init(&lis2mdl->gpio_sem, 0, UINT_MAX);
	k_thread_create(&lis2mdl->thread, lis2mdl->thread_stack,
			CONFIG_LIS2MDL_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis2mdl_thread, lis2mdl,
			NULL, NULL, K_PRIO_COOP(CONFIG_LIS2MDL_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD)
	lis2mdl->work.handler = lis2mdl_work_cb;
#endif

	gpio_pin_configure(lis2mdl->gpio, config->gpio_pin,
			   GPIO_INPUT | config->gpio_flags);

	gpio_init_callback(&lis2mdl->gpio_cb,
			   lis2mdl_gpio_callback,
			   BIT(config->gpio_pin));

	if (gpio_add_callback(lis2mdl->gpio, &lis2mdl->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	return gpio_pin_interrupt_configure(lis2mdl->gpio, config->gpio_pin,
					    GPIO_INT_EDGE_TO_ACTIVE);
}
