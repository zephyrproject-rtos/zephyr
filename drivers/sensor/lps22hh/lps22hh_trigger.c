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

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lps22hh.h"

LOG_MODULE_DECLARE(LPS22HH, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lps22hh_enable_int - enable selected int pin to generate interrupt
 */
static int lps22hh_enable_int(const struct device *dev, int enable)
{
	const struct lps22hh_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps22hh_pin_int_route_t int_route;

	/* set interrupt */
	lps22hh_pin_int_route_get(ctx, &int_route);
	int_route.drdy_pres = enable;
	return lps22hh_pin_int_route_set(ctx, &int_route);
}

/**
 * lps22hh_trigger_set - link external trigger to event data ready
 */
int lps22hh_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct lps22hh_data *lps22hh = dev->data;
	const struct lps22hh_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint32_t raw_press;

	if (trig->chan == SENSOR_CHAN_ALL) {
		lps22hh->handler_drdy = handler;
		lps22hh->data_ready_trigger = trig;
		if (handler) {
			/* dummy read: re-trigger interrupt */
			if (lps22hh_pressure_raw_get(ctx, &raw_press) < 0) {
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
	int ret;
	struct lps22hh_data *lps22hh = dev->data;
	const struct lps22hh_config *cfg = dev->config;

	if (lps22hh->handler_drdy != NULL) {
		lps22hh->handler_drdy(dev, lps22hh->data_ready_trigger);
	}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	if (cfg->i3c.bus != NULL) {
		/*
		 * I3C IBI does not rely on GPIO.
		 * So no need to enable GPIO pin for interrupt trigger.
		 */
		return;
	}
#endif

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}
}

static void lps22hh_intr_callback(struct lps22hh_data *lps22hh)
{
#if defined(CONFIG_LPS22HH_TRIGGER_OWN_THREAD)
	k_sem_give(&lps22hh->intr_sem);
#elif defined(CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lps22hh->work);
#endif /* CONFIG_LPS22HH_TRIGGER_OWN_THREAD */
}

static void lps22hh_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct lps22hh_data *lps22hh =
		CONTAINER_OF(cb, struct lps22hh_data, gpio_cb);

	ARG_UNUSED(pins);
	const struct lps22hh_config *cfg = lps22hh->dev->config;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}

	lps22hh_intr_callback(lps22hh);
}

#ifdef CONFIG_LPS22HH_TRIGGER_OWN_THREAD
static void lps22hh_thread(struct lps22hh_data *lps22hh)
{
	while (1) {
		k_sem_take(&lps22hh->intr_sem, K_FOREVER);
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

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
static int lps22hh_ibi_cb(struct i3c_device_desc *target,
			  struct i3c_ibi_payload *payload)
{
	const struct device *dev = target->dev;
	struct lps22hh_data *lps22hh = dev->data;

	ARG_UNUSED(payload);

	lps22hh_intr_callback(lps22hh);

	return 0;
}
#endif

int lps22hh_init_interrupt(const struct device *dev)
{
	struct lps22hh_data *lps22hh = dev->data;
	const struct lps22hh_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	/* setup data ready gpio interrupt */
	if (!device_is_ready(cfg->gpio_int.port)
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	    && (cfg->i3c.bus == NULL)
#endif
	   ) {
		if (cfg->gpio_int.port) {
			LOG_ERR("%s: device %s is not ready", dev->name,
						cfg->gpio_int.port->name);
			return -ENODEV;
		}

		LOG_DBG("%s: gpio_int not defined in DT", dev->name);
		return 0;
	}

	lps22hh->dev = dev;

#if defined(CONFIG_LPS22HH_TRIGGER_OWN_THREAD)
	k_sem_init(&lps22hh->intr_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lps22hh->thread, lps22hh->thread_stack,
		       CONFIG_LPS22HH_THREAD_STACK_SIZE,
		       (k_thread_entry_t)lps22hh_thread, lps22hh,
		       NULL, NULL, K_PRIO_COOP(CONFIG_LPS22HH_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD)
	lps22hh->work.handler = lps22hh_work_cb;
#endif /* CONFIG_LPS22HH_TRIGGER_OWN_THREAD */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	if (cfg->i3c.bus == NULL)
#endif
	{
		ret = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure gpio");
			return ret;
		}

		LOG_INF("%s: int on %s.%02u", dev->name, cfg->gpio_int.port->name,
					      cfg->gpio_int.pin);

		gpio_init_callback(&lps22hh->gpio_cb,
				   lps22hh_gpio_callback,
				   BIT(cfg->gpio_int.pin));

		ret = gpio_add_callback(cfg->gpio_int.port, &lps22hh->gpio_cb);
		if (ret < 0) {
			LOG_ERR("Could not set gpio callback");
			return ret;
		}
	}

	/* enable interrupt in pulse mode */
	if (lps22hh_int_notification_set(ctx, LPS22HH_INT_PULSED) < 0) {
		return -EIO;
	}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	if (cfg->i3c.bus != NULL) {
		/* I3C IBI does not utilize GPIO interrupt. */
		lps22hh->i3c_dev->ibi_cb = lps22hh_ibi_cb;

		if (i3c_ibi_enable(lps22hh->i3c_dev) != 0) {
			LOG_DBG("Could not enable I3C IBI");
			return -EIO;
		}

		return 0;
	}
#endif

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
