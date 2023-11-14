/* ST Microelectronics LPS28DFW pressure and temperature sensor
 *
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps28dfw.pdf
 */

#define DT_DRV_COMPAT st_lps28dfw

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lps28dfw.h"

LOG_MODULE_DECLARE(LPS28DFW, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lps28dfw_enable_int - enable selected int pin to generate interrupt
 */
static int lps28dfw_enable_int(const struct device *dev, int enable)
{
	const struct lps28dfw_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_pin_int_route_t int_route;

	/* set interrupt */
	lps28dfw_pin_int_route_get(ctx, &int_route);
	int_route.drdy_pres = enable;
	return lps28dfw_pin_int_route_set(ctx, &int_route);
}

/**
 * lps28dfw_trigger_set - link external trigger to event data ready
 */
int lps28dfw_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct lps28dfw_data *lps28dfw = dev->data;
	const struct lps28dfw_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_data_t raw_data;
	lps28dfw_md_t md;

	if (trig->chan != SENSOR_CHAN_ALL) {
		LOG_WRN("trigger set not supported on this channel.");
		return -ENOTSUP;
	}

	lps28dfw->handler_drdy = handler;
	lps28dfw->data_ready_trigger = trig;
	if (handler) {
		/* dummy read: re-trigger interrupt */
		if (lps28dfw_data_get(ctx, &md, &raw_data) < 0) {
			LOG_DBG("Failed to read sample");
			return -EIO;
		}
		return lps28dfw_enable_int(dev, 1);
	} else {
		return lps28dfw_enable_int(dev, 0);
	}

	return -ENOTSUP;
}

/**
 * lps28dfw_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lps28dfw_handle_interrupt(const struct device *dev)
{
	int ret;
	struct lps28dfw_data *lps28dfw = dev->data;
	const struct lps28dfw_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_all_sources_t status;

	if (lps28dfw_all_sources_get(ctx, &status) < 0) {
		LOG_DBG("failed reading status reg");
		goto exit;
	}

	if (status.drdy_pres == 0) {
		goto exit; /* spurious interrupt */
	}

	if (lps28dfw->handler_drdy != NULL) {
		lps28dfw->handler_drdy(dev, lps28dfw->data_ready_trigger);
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

static void lps28dfw_intr_callback(struct lps28dfw_data *lps28dfw)
{
#if defined(CONFIG_LPS28DFW_TRIGGER_OWN_THREAD)
	k_sem_give(&lps28dfw->intr_sem);
#elif defined(CONFIG_LPS28DFW_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lps28dfw->work);
#endif /* CONFIG_LPS28DFW_TRIGGER_OWN_THREAD */
}

static void lps28dfw_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct lps28dfw_data *lps28dfw =
		CONTAINER_OF(cb, struct lps28dfw_data, gpio_cb);

	ARG_UNUSED(pins);
	const struct lps28dfw_config *cfg = lps28dfw->dev->config;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}

	lps28dfw_intr_callback(lps28dfw);
}

#ifdef CONFIG_LPS28DFW_TRIGGER_OWN_THREAD
static void lps28dfw_thread(struct lps28dfw_data *lps28dfw)
{
	while (1) {
		k_sem_take(&lps28dfw->intr_sem, K_FOREVER);
		lps28dfw_handle_interrupt(lps28dfw->dev);
	}
}
#endif /* CONFIG_LPS28DFW_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LPS28DFW_TRIGGER_GLOBAL_THREAD
static void lps28dfw_work_cb(struct k_work *work)
{
	struct lps28dfw_data *lps28dfw =
		CONTAINER_OF(work, struct lps28dfw_data, work);

	lps28dfw_handle_interrupt(lps28dfw->dev);
}
#endif /* CONFIG_LPS28DFW_TRIGGER_GLOBAL_THREAD */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
static int lps28dfw_ibi_cb(struct i3c_device_desc *target,
			  struct i3c_ibi_payload *payload)
{
	const struct device *dev = target->dev;
	struct lps28dfw_data *lps28dfw = dev->data;

	ARG_UNUSED(payload);

	lps28dfw_intr_callback(lps28dfw);

	return 0;
}
#endif

int lps28dfw_init_interrupt(const struct device *dev)
{
	struct lps28dfw_data *lps28dfw = dev->data;
	const struct lps28dfw_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_int_mode_t mode;
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

	lps28dfw->dev = dev;

#if defined(CONFIG_LPS28DFW_TRIGGER_OWN_THREAD)
	k_sem_init(&lps28dfw->intr_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lps28dfw->thread, lps28dfw->thread_stack,
		       CONFIG_LPS28DFW_THREAD_STACK_SIZE,
		       (k_thread_entry_t)lps28dfw_thread, lps28dfw,
		       NULL, NULL, K_PRIO_COOP(CONFIG_LPS28DFW_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_LPS28DFW_TRIGGER_GLOBAL_THREAD)
	lps28dfw->work.handler = lps28dfw_work_cb;
#endif /* CONFIG_LPS28DFW_TRIGGER_OWN_THREAD */

	if (!ON_I3C_BUS(cfg)) {
		ret = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure gpio");
			return ret;
		}

		LOG_INF("%s: int on %s.%02u", dev->name, cfg->gpio_int.port->name,
					      cfg->gpio_int.pin);

		gpio_init_callback(&lps28dfw->gpio_cb,
				   lps28dfw_gpio_callback,
				   BIT(cfg->gpio_int.pin));

		ret = gpio_add_callback(cfg->gpio_int.port, &lps28dfw->gpio_cb);
		if (ret < 0) {
			LOG_ERR("Could not set gpio callback");
			return ret;
		}
	}

	/* enable drdy in pulsed/latched mode */
	LOG_DBG("drdy_pulsed is %d", (int)cfg->drdy_pulsed);
	mode.drdy_latched = ~cfg->drdy_pulsed;
	if (lps28dfw_interrupt_mode_set(ctx, &mode) < 0) {
		return -EIO;
	}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	if (cfg->i3c.bus != NULL) {
		/* I3C IBI does not utilize GPIO interrupt. */
		lps28dfw->i3c_dev->ibi_cb = lps28dfw_ibi_cb;

		if (i3c_ibi_enable(lps28dfw->i3c_dev) != 0) {
			LOG_DBG("Could not enable I3C IBI");
			return -EIO;
		}

		return 0;
	}
#endif

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					       GPIO_INT_EDGE_TO_ACTIVE);
}
