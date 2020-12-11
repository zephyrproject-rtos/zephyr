/* ST Microelectronics LPS22HH pressure and temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22hh.pdf
 */

#define DT_DRV_COMPAT st_lps22hh

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "lps22hh.h"

LOG_MODULE_DECLARE(LPS22HH, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lps22hh_enable_int - enable selected int pin to generate interrupt
 */
static int lps22hh_enable_int(const struct device *dev, int enable)
{
	struct lps22hh_data *lps22hh = dev->data;
	lps22hh_reg_t int_route;

	/* set interrupt */
	lps22hh_pin_int_route_get(lps22hh->ctx,
				   &int_route.ctrl_reg3);
	int_route.ctrl_reg3.drdy = enable;
	return lps22hh_pin_int_route_set(lps22hh->ctx,
					  &int_route.ctrl_reg3);
}

/**
 * lps22hh_trigger_set - link external trigger to event data ready
 */
int lps22hh_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct lps22hh_data *lps22hh = dev->data;
	uint32_t raw_press;

	if (trig->chan == SENSOR_CHAN_ALL) {
		lps22hh->handler_drdy = handler;
		if (handler) {
			/* dummy read: re-trigger interrupt */
			if (lps22hh_pressure_raw_get(lps22hh->ctx,
			    &raw_press) < 0) {
				LOG_DBG("Failed to read sample");
				return -EIO;
			}
			return lps22hh_enable_int(dev, 1);
		} else {
			return lps22hh_enable_int(dev, 0);
		}
	}

	return -ENOTSUP;
}

/**
 * lps22hh_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lps22hh_handle_interrupt(const struct device *dev)
{
	struct lps22hh_data *lps22hh = dev->data;
	const struct lps22hh_config *cfg = dev->config;
	struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
	};

	if (lps22hh->handler_drdy != NULL) {
		lps22hh->handler_drdy(dev, &drdy_trigger);
	}

	gpio_pin_interrupt_configure(lps22hh->gpio, cfg->drdy_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

static void lps22hh_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct lps22hh_data *lps22hh =
		CONTAINER_OF(cb, struct lps22hh_data, gpio_cb);

	ARG_UNUSED(pins);
	const struct lps22hh_config *cfg = lps22hh->dev->config;

	gpio_pin_interrupt_configure(lps22hh->gpio, cfg->drdy_pin,
				     GPIO_INT_DISABLE);

#if defined(CONFIG_LPS22HH_TRIGGER_OWN_THREAD)
	k_sem_give(&lps22hh->gpio_sem);
#elif defined(CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lps22hh->work);
#endif /* CONFIG_LPS22HH_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LPS22HH_TRIGGER_OWN_THREAD
static void lps22hh_thread(struct lps22hh_data *lps22hh)
{
	while (1) {
		k_sem_take(&lps22hh->gpio_sem, K_FOREVER);
		lps22hh_handle_interrupt(lps22hh->dev);
	}
}
#endif /* CONFIG_LPS22HH_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD
static void lps22hh_work_cb(struct k_work *work)
{
	struct lps22hh_data *lps22hh =
		CONTAINER_OF(work, struct lps22hh_data, work);

	lps22hh_handle_interrupt(lps22hh->dev);
}
#endif /* CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD */

int lps22hh_init_interrupt(const struct device *dev)
{
	struct lps22hh_data *lps22hh = dev->data;
	const struct lps22hh_config *cfg = dev->config;
	int ret;

	/* setup data ready gpio interrupt */
	lps22hh->gpio = device_get_binding(cfg->drdy_port);
	if (lps22hh->gpio == NULL) {
		LOG_DBG("Cannot get pointer to %s device", cfg->drdy_port);
		return -EINVAL;
	}

#if defined(CONFIG_LPS22HH_TRIGGER_OWN_THREAD)
	k_sem_init(&lps22hh->gpio_sem, 0, UINT_MAX);

	k_thread_create(&lps22hh->thread, lps22hh->thread_stack,
		       CONFIG_LPS22HH_THREAD_STACK_SIZE,
		       (k_thread_entry_t)lps22hh_thread, lps22hh,
		       NULL, NULL, K_PRIO_COOP(CONFIG_LPS22HH_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD)
	lps22hh->work.handler = lps22hh_work_cb;
#endif /* CONFIG_LPS22HH_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure(lps22hh->gpio, cfg->drdy_pin,
				 GPIO_INPUT | cfg->drdy_flags);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&lps22hh->gpio_cb, lps22hh_gpio_callback,
			   BIT(cfg->drdy_pin));

	if (gpio_add_callback(lps22hh->gpio, &lps22hh->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* enable interrupt in pulse mode */
	if (lps22hh_int_notification_set(lps22hh->ctx,
					 LPS22HH_INT_PULSED) < 0) {
		return -EIO;
	}

	return gpio_pin_interrupt_configure(lps22hh->gpio, cfg->drdy_pin,
					    GPIO_INT_EDGE_TO_ACTIVE);
}
