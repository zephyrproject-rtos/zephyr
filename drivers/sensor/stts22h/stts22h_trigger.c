/* ST Microelectronics STTS22H temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
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
 * stts22h_enable_int - enable selected int pin to generate interrupt
 */
static int stts22h_enable_int(const struct device *dev, int enable)
{
	struct stts22h_data *stts22h = dev->data;
	uint8_t en = (enable) ? 0 : 1;

	return stts22h_pin_event_route_set(stts22h->ctx, en);
}

/**
 * stts22h_trigger_set - link external trigger to event data ready
 */
int stts22h_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct stts22h_data *stts22h = dev->data;
	const struct stts22h_config *config = dev->config;

	if (!config->int_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->chan == SENSOR_CHAN_ALL) {
		stts22h->thsld_handler = handler;
		stts22h->thsld_trigger = trig;
		if (handler) {
			return stts22h_enable_int(dev, 1);
		} else {
			return stts22h_enable_int(dev, 0);
		}
	}

	return -ENOTSUP;
}

/**
 * stts22h_handle_interrupt - handle the thsld event
 * read data and call handler if registered any
 */
static void stts22h_handle_interrupt(const struct device *dev)
{
	struct stts22h_data *stts22h = dev->data;
	const struct stts22h_config *cfg = dev->config;
	stts22h_status_t status;

	stts22h_dev_status_get(stts22h->ctx, &status);

	if (stts22h->thsld_handler != NULL && (status.over_thh || status.under_thl)) {
		stts22h->thsld_handler(dev, stts22h->thsld_trigger);
	}

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

static void stts22h_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct stts22h_data *stts22h = CONTAINER_OF(cb, struct stts22h_data, gpio_cb);
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
	struct stts22h_data *stts22h = CONTAINER_OF(work, struct stts22h_data, work);

	stts22h_handle_interrupt(stts22h->dev);
}
#endif /* CONFIG_STTS22H_TRIGGER_GLOBAL_THREAD */

int stts22h_init_interrupt(const struct device *dev)
{
	struct stts22h_data *stts22h = dev->data;
	const struct stts22h_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

#if defined(CONFIG_STTS22H_TRIGGER_OWN_THREAD)
	k_sem_init(&stts22h->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&stts22h->thread, stts22h->thread_stack, CONFIG_STTS22H_THREAD_STACK_SIZE,
			stts22h_thread, stts22h, NULL, NULL,
			K_PRIO_COOP(CONFIG_STTS22H_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_STTS22H_TRIGGER_GLOBAL_THREAD)
	stts22h->work.handler = stts22h_work_cb;
#endif /* CONFIG_STTS22H_TRIGGER_OWN_THREAD */

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

	/* Enable interrupt on high temperature */
	float temp_hi = (float)CONFIG_STTS22H_TEMP_HI_THRESHOLD;
	float temp_lo = (float)CONFIG_STTS22H_TEMP_LO_THRESHOLD;

	stts22h_high_temperature_threshold_set(stts22h->ctx, stts22h_from_celsius_to_lsb(temp_hi));

	stts22h_low_temperature_threshold_set(stts22h->ctx, stts22h_from_celsius_to_lsb(temp_lo));

	return gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
