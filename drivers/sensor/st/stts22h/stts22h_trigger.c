/* ST Microelectronics STTS22H temperature sensor
 *
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts22h.pdf
 */

#define DT_DRV_COMPAT st_stts22h

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "stts22h.h"

LOG_MODULE_DECLARE(STTS22H, CONFIG_SENSOR_LOG_LEVEL);

/**
 * stts22h_trigger_set - link external trigger to event data ready
 */
int stts22h_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct stts22h_data *stts22h = dev->data;
	const struct stts22h_config *config = dev->config;

	if (!config->int_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->chan != SENSOR_CHAN_ALL &&
	    trig->chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Unsupported sensor trigger %d", trig->chan);
		return -ENOTSUP;
	}

	stts22h->thsld_handler = handler;
	stts22h->thsld_trigger = trig;

	return 0;
}

/**
 * stts22h_handle_interrupt - handle the thsld event
 * read data and call handler if registered any
 */
static void stts22h_handle_interrupt(const struct device *dev)
{
	struct stts22h_data *stts22h = dev->data;
	const struct stts22h_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	stts22h_temp_trlhd_src_t status = {0};
	int ret;

	/* PM turn-off ran after the interrupt fired: the chip has no supply */
	if (atomic_get(&stts22h->int_enabled) == 0) {
		return;
	}

	if (stts22h_temp_trshld_src_get(ctx, &status) < 0) {
		LOG_ERR("Failed to read threshold status");
	}

	if (stts22h->thsld_handler != NULL &&
	    (status.under_thl || status.over_thh)) {
		stts22h->thsld_handler(dev, stts22h->thsld_trigger);
	}

	/* PM turn-off may have run while the status read or the handler blocked */
	if (atomic_get(&stts22h->int_enabled) == 0) {
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not re-arm interrupt: %d", ret);
		return;
	}

	/* PM turn-off may also have run between the check above and the re-arm */
	if (atomic_get(&stts22h->int_enabled) == 0) {
		ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
		if (ret < 0) {
			LOG_ERR("Could not disarm interrupt: %d", ret);
		}
	}
}

static void stts22h_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct stts22h_data *stts22h =
		CONTAINER_OF(cb, struct stts22h_data, gpio_cb);
	const struct stts22h_config *cfg = stts22h->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_STTS22H_TRIGGER_OWN_THREAD)
	k_sem_give(&stts22h->gpio_sem);
#elif defined(CONFIG_STTS22H_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&stts22h->work);
#endif /* CONFIG_STTS22H_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_STTS22H_TRIGGER_OWN_THREAD
static void stts22h_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct stts22h_data *stts22h = p1;

	while (1) {
		k_sem_take(&stts22h->gpio_sem, K_FOREVER);
		stts22h_handle_interrupt(stts22h->dev);
	}
}
#endif /* CONFIG_STTS22H_TRIGGER_OWN_THREAD */

#ifdef CONFIG_STTS22H_TRIGGER_GLOBAL_THREAD
static void stts22h_work_cb(struct k_work *work)
{
	struct stts22h_data *stts22h =
		CONTAINER_OF(work, struct stts22h_data, work);

	stts22h_handle_interrupt(stts22h->dev);
}
#endif /* CONFIG_STTS22H_TRIGGER_GLOBAL_THREAD */

int stts22h_init_interrupt(const struct device *dev)
{
	struct stts22h_data *stts22h = dev->data;
	const struct stts22h_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->int_gpio.port);
		return -ENODEV;
	}

	/*
	 * The GPIO steps can fail and are safe to repeat; the thread or work item
	 * is created last, so a failed init leaves nothing that a retry would
	 * create twice. The interrupt is armed only by PM turn-on, later.
	 */
	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&stts22h->gpio_cb, stts22h_gpio_callback, BIT(cfg->int_gpio.pin));

	if (gpio_add_callback(cfg->int_gpio.port, &stts22h->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

#if defined(CONFIG_STTS22H_TRIGGER_OWN_THREAD)
	k_sem_init(&stts22h->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&stts22h->thread, stts22h->thread_stack,
			CONFIG_STTS22H_THREAD_STACK_SIZE,
			stts22h_thread, stts22h,
			NULL, NULL, K_PRIO_COOP(CONFIG_STTS22H_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&stts22h->thread, dev->name);
#elif defined(CONFIG_STTS22H_TRIGGER_GLOBAL_THREAD)
	stts22h->work.handler = stts22h_work_cb;
#endif /* CONFIG_STTS22H_TRIGGER_OWN_THREAD */

	return 0;
}

/**
 * stts22h_config_interrupt - program the thresholds and enable the
 * interrupt, or disable the interrupt
 */
int stts22h_config_interrupt(const struct device *dev, bool enable)
{
	struct stts22h_data *stts22h = dev->data;
	const struct stts22h_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	stts22h_temp_trlhd_src_t status;
	int ret;

	if (!enable) {
		/* Cleared first so that a running handler does not re-arm */
		atomic_set(&stts22h->int_enabled, 0);
		/*
		 * A GPIO expander may block here, which the idle thread and ISRs
		 * cannot; there the first INT edge disarms through the GPIO callback.
		 */
		if (!k_can_yield()) {
			return 0;
		}
		return gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
	}

	/* Enable interrupt on high/low temperature threshold */
	if (stts22h_temp_trshld_high_set(ctx, stts22h->temp_hi) < 0) {
		LOG_DBG("Could not set high threshold");
		return -EIO;
	}

	if (stts22h_temp_trshld_low_set(ctx, stts22h->temp_lo) < 0) {
		LOG_DBG("Could not set low threshold");
		return -EIO;
	}

	/* Set before arming so that the first interrupt's handler re-arms */
	atomic_set(&stts22h->int_enabled, 1);
	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		/* Not ret: pm_device_driver_init() would skip a TURN_ON returning -ENOTSUP */
		LOG_DBG("Could not arm interrupt: %d", ret);
		atomic_set(&stts22h->int_enabled, 0);
		return -EIO;
	}

	/*
	 * INT stays asserted until STATUS is read, also across an MCU reset, and
	 * an edge interrupt armed on an asserted line never fires.
	 */
	if (stts22h_temp_trshld_src_get(ctx, &status) < 0) {
		LOG_DBG("Could not read threshold status");
		/* A failed TURN_ON leaves the interrupt disarmed */
		atomic_set(&stts22h->int_enabled, 0);
		(void)gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
		return -EIO;
	}

	return 0;
}
