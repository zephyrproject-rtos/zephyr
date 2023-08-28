/* ST Microelectronics LPS22DF pressure and temperature sensor
 *
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22df.pdf
 */

#define DT_DRV_COMPAT st_lps22df

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lps22df.h"

LOG_MODULE_DECLARE(LPS22DF, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lps22df_enable_int - enable selected int pin to generate interrupt
 */
static int lps22df_enable_int(const struct device *dev, int enable)
{
	const struct lps22df_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps22df_pin_int_route_t int_route;

	/* set interrupt */
	lps22df_pin_int_route_get(ctx, &int_route);
	int_route.drdy_pres = enable;
	return lps22df_pin_int_route_set(ctx, &int_route);
}

/**
 * lps22df_trigger_set - link external trigger to event data ready
 */
int lps22df_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct lps22df_data *lps22df = dev->data;
	const struct lps22df_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps22df_data_t raw_data;

	if (trig->chan != SENSOR_CHAN_ALL) {
		LOG_WRN("trigger set not supported on this channel.");
		return -ENOTSUP;
	}

	lps22df->handler_drdy = handler;
	lps22df->data_ready_trigger = trig;
	if (handler) {
		/* dummy read: re-trigger interrupt */
		if (lps22df_data_get(ctx, &raw_data) < 0) {
			LOG_DBG("Failed to read sample");
			return -EIO;
		}
		return lps22df_enable_int(dev, 1);
	} else {
		return lps22df_enable_int(dev, 0);
	}

	return -ENOTSUP;
}

/**
 * lps22df_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lps22df_handle_interrupt(const struct device *dev)
{
	int ret;
	struct lps22df_data *lps22df = dev->data;
	const struct lps22df_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps22df_all_sources_t status;

	if (lps22df_all_sources_get(ctx, &status) < 0) {
		LOG_DBG("failed reading status reg");
		goto exit;
	}

	if (status.drdy_pres == 0) {
		goto exit; /* spurious interrupt */
	}

	if (lps22df->handler_drdy != NULL) {
		lps22df->handler_drdy(dev, lps22df->data_ready_trigger);
	}

	if (ON_I3C_BUS(cfg)) {
		/*
		 * I3C IBI does not rely on GPIO.
		 * So no need to enable GPIO pin for interrupt trigger.
		 */
		return;
	}

exit:
	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}
}

static void lps22df_intr_callback(struct lps22df_data *lps22df)
{
#if defined(CONFIG_LPS22DF_TRIGGER_OWN_THREAD)
	k_sem_give(&lps22df->intr_sem);
#elif defined(CONFIG_LPS22DF_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lps22df->work);
#endif /* CONFIG_LPS22DF_TRIGGER_OWN_THREAD */
}

static void lps22df_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct lps22df_data *lps22df =
		CONTAINER_OF(cb, struct lps22df_data, gpio_cb);

	ARG_UNUSED(pins);
	const struct lps22df_config *cfg = lps22df->dev->config;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}

	lps22df_intr_callback(lps22df);
}

#ifdef CONFIG_LPS22DF_TRIGGER_OWN_THREAD
static void lps22df_thread(struct lps22df_data *lps22df)
{
	while (1) {
		k_sem_take(&lps22df->intr_sem, K_FOREVER);
		lps22df_handle_interrupt(lps22df->dev);
	}
}
#endif /* CONFIG_LPS22DF_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LPS22DF_TRIGGER_GLOBAL_THREAD
static void lps22df_work_cb(struct k_work *work)
{
	struct lps22df_data *lps22df =
		CONTAINER_OF(work, struct lps22df_data, work);

	lps22df_handle_interrupt(lps22df->dev);
}
#endif /* CONFIG_LPS22DF_TRIGGER_GLOBAL_THREAD */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
static int lps22df_ibi_cb(struct i3c_device_desc *target,
			  struct i3c_ibi_payload *payload)
{
	const struct device *dev = target->dev;
	struct lps22df_data *lps22df = dev->data;

	ARG_UNUSED(payload);

	lps22df_intr_callback(lps22df);

	return 0;
}
#endif

int lps22df_init_interrupt(const struct device *dev)
{
	struct lps22df_data *lps22df = dev->data;
	const struct lps22df_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps22df_int_mode_t mode;
	int ret;

	/* setup data ready gpio interrupt */
	if (!gpio_is_ready_dt(&cfg->gpio_int) && !ON_I3C_BUS(cfg)) {
		if (cfg->gpio_int.port) {
			LOG_ERR("%s: device %s is not ready", dev->name,
						cfg->gpio_int.port->name);
			return -ENODEV;
		}

		LOG_DBG("%s: gpio_int not defined in DT", dev->name);
		return 0;
	}

	lps22df->dev = dev;

#if defined(CONFIG_LPS22DF_TRIGGER_OWN_THREAD)
	k_sem_init(&lps22df->intr_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lps22df->thread, lps22df->thread_stack,
		       CONFIG_LPS22DF_THREAD_STACK_SIZE,
		       (k_thread_entry_t)lps22df_thread, lps22df,
		       NULL, NULL, K_PRIO_COOP(CONFIG_LPS22DF_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_LPS22DF_TRIGGER_GLOBAL_THREAD)
	lps22df->work.handler = lps22df_work_cb;
#endif /* CONFIG_LPS22DF_TRIGGER_OWN_THREAD */

	if (!ON_I3C_BUS(cfg)) {
		ret = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure gpio");
			return ret;
		}

		LOG_INF("%s: int on %s.%02u", dev->name, cfg->gpio_int.port->name,
					      cfg->gpio_int.pin);

		gpio_init_callback(&lps22df->gpio_cb,
				   lps22df_gpio_callback,
				   BIT(cfg->gpio_int.pin));

		ret = gpio_add_callback(cfg->gpio_int.port, &lps22df->gpio_cb);
		if (ret < 0) {
			LOG_ERR("Could not set gpio callback");
			return ret;
		}
	}

	/* enable drdy in pulsed/latched mode */
	LOG_DBG("drdy_pulsed is %d", (int)cfg->drdy_pulsed);
	mode.drdy_latched = ~cfg->drdy_pulsed;
	if (lps22df_interrupt_mode_set(ctx, &mode) < 0) {
		return -EIO;
	}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	if (cfg->i3c.bus != NULL) {
		/* I3C IBI does not utilize GPIO interrupt. */
		lps22df->i3c_dev->ibi_cb = lps22df_ibi_cb;

		if (i3c_ibi_enable(lps22df->i3c_dev) != 0) {
			LOG_DBG("Could not enable I3C IBI");
			return -EIO;
		}

		return 0;
	}
#endif

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
