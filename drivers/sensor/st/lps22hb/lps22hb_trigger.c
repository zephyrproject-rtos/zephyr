/* ST Microelectronics LPS22HB pressure and temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22hb.pdf
 */

#define DT_DRV_COMPAT st_lps22hb_press

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "lps22hb.h"

LOG_MODULE_DECLARE(LPS22HB, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lps22hb_enable_int - enable selected int pin to generate interrupt
 */
static int lps22hb_enable_int(const struct device *dev, int enable)
{
	const struct lps22hb_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	/* set interrupt */
	int rc = lps22hb_int_generation_set(ctx, enable);
	if (rc != 0) {
		LOG_ERR("Failed to enable interrupt");
		return -EIO;
	}

	LOG_INF("%s interrupts", enable ? "Enable" : "Disable");
	return 0;
}

/**
 * lps22hb_trigger_set - link external trigger to event data ready
 */
int lps22hb_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct lps22hb_data *lps22hb = dev->data;
	const struct lps22hb_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint32_t raw_press;

	if (trig->chan == SENSOR_CHAN_ALL) {
		lps22hb->handler_drdy = handler;
		lps22hb->data_ready_trigger = trig;
		if (handler) {
			/* dummy read: re-trigger interrupt */
			if (lps22hb_pressure_raw_get(ctx, &raw_press) < 0) {
				LOG_ERR("Failed to read sample");
				return -EIO;
			}
			return lps22hb_enable_int(dev, 1);
		} else {
			return lps22hb_enable_int(dev, 0);
		}
	}

	return -ENOTSUP;
}

/**
 * lps22hb_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lps22hb_handle_interrupt(const struct device *dev)
{
	int ret;
	struct lps22hb_data *lps22hb = dev->data;
	const struct lps22hb_config *cfg = dev->config;

	if (lps22hb->handler_drdy != NULL) {
		lps22hb->handler_drdy(dev, lps22hb->data_ready_trigger);
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}
}

static void lps22hb_intr_callback(struct lps22hb_data *lps22hb)
{
#if defined(CONFIG_LPS22HB_TRIGGER_OWN_THREAD)
	k_sem_give(&lps22hb->intr_sem);
#elif defined(CONFIG_LPS22HB_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lps22hb->work);
#endif /* CONFIG_LPS22HB_TRIGGER_OWN_THREAD */
}

static void lps22hb_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct lps22hb_data *lps22hb = CONTAINER_OF(cb, struct lps22hb_data, gpio_cb);

	ARG_UNUSED(pins);
	const struct lps22hb_config *cfg = lps22hb->dev->config;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}

	lps22hb_intr_callback(lps22hb);
}

#ifdef CONFIG_LPS22HB_TRIGGER_OWN_THREAD
static void lps22hb_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct lps22hb_data *lps22hb = p1;

	while (1) {
		k_sem_take(&lps22hb->intr_sem, K_FOREVER);
		lps22hb_handle_interrupt(lps22hb->dev);
	}
}
#endif /* CONFIG_LPS22HB_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LPS22HB_TRIGGER_GLOBAL_THREAD
static void lps22hb_work_cb(struct k_work *work)
{
	struct lps22hb_data *lps22hb = CONTAINER_OF(work, struct lps22hb_data, work);

	lps22hb_handle_interrupt(lps22hb->dev);
}
#endif /* CONFIG_LPS22HB_TRIGGER_GLOBAL_THREAD */

int lps22hb_init_interrupt(const struct device *dev)
{
	struct lps22hb_data *lps22hb = dev->data;
	const struct lps22hb_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	/* setup data ready gpio interrupt */
	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		if (cfg->gpio_int.port) {
			LOG_ERR("%s: device %s is not ready", dev->name, cfg->gpio_int.port->name);
			return -ENODEV;
		}

		LOG_DBG("%s: gpio_int not defined in DT", dev->name);
		return 0;
	}

	lps22hb->dev = dev;

#if defined(CONFIG_LPS22HB_TRIGGER_OWN_THREAD)
	k_sem_init(&lps22hb->intr_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&lps22hb->thread, lps22hb->thread_stack, CONFIG_LPS22HB_THREAD_STACK_SIZE,
			lps22hb_thread, lps22hb, NULL, NULL,
			K_PRIO_COOP(CONFIG_LPS22HB_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_LPS22HB_TRIGGER_GLOBAL_THREAD)
	lps22hb->work.handler = lps22hb_work_cb;
#endif /* CONFIG_LPS22HB_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	LOG_INF("%s: int on %s.%02u", dev->name, cfg->gpio_int.port->name, cfg->gpio_int.pin);

	gpio_init_callback(&lps22hb->gpio_cb, lps22hb_gpio_callback, BIT(cfg->gpio_int.pin));

	ret = gpio_add_callback(cfg->gpio_int.port, &lps22hb->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return ret;
	}

	/* enable interrupt in pulse mode */
	LOG_DBG("Configuring interrupts");
	int rc = lps22hb_int_notification_mode_set(ctx, LPS22HB_INT_PULSED);
	rc += lps22hb_stop_on_fifo_threshold_set(ctx, 0);
	rc += lps22hb_fifo_mode_set(ctx, LPS22HB_BYPASS_MODE);
	rc += lps22hb_drdy_on_int_set(ctx, 1);
	rc += lps22hb_int_pin_mode_set(ctx, LPS22HB_DRDY_OR_FIFO_FLAGS);
	rc += lps22hb_pin_mode_set(ctx, LPS22HB_OPEN_DRAIN);
	rc += lps22hb_int_polarity_set(ctx, LPS22HB_ACTIVE_HIGH);
	
	if (rc != 0) {
		LOG_ERR("Failed to configure interrupt");
		return -EIO;
	}

	return gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
}
