/* ST Microelectronics STTS751 temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts751.pdf
 */

#define DT_DRV_COMPAT st_stts751

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "stts751.h"

LOG_MODULE_DECLARE(STTS751, CONFIG_SENSOR_LOG_LEVEL);

/**
 * stts751_enable_int - enable selected int pin to generate interrupt
 */
static int stts751_enable_int(const struct device *dev, int enable)
{
	struct stts751_data *stts751 = dev->data;
	uint8_t en = (enable) ? 0 : 1;

	return stts751_pin_event_route_set(stts751->ctx, en);
}

/**
 * stts751_trigger_set - link external trigger to event data ready
 */
int stts751_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct stts751_data *stts751 = dev->data;
	const struct stts751_config *config = dev->config;

	if (!config->int_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->chan == SENSOR_CHAN_ALL) {
		stts751->thsld_handler = handler;
		if (handler) {
			return stts751_enable_int(dev, 1);
		} else {
			return stts751_enable_int(dev, 0);
		}
	}

	return -ENOTSUP;
}

/**
 * stts751_handle_interrupt - handle the thsld event
 * read data and call handler if registered any
 */
static void stts751_handle_interrupt(const struct device *dev)
{
	struct stts751_data *stts751 = dev->data;
	const struct stts751_config *cfg = dev->config;
	struct sensor_trigger thsld_trigger = {
		.type = SENSOR_TRIG_THRESHOLD,
	};
	stts751_status_t status;

	stts751_status_reg_get(stts751->ctx, &status);

	if (stts751->thsld_handler != NULL &&
	    (status.t_high || status.t_low)) {
		stts751->thsld_handler(dev, &thsld_trigger);
	}

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

static void stts751_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct stts751_data *stts751 =
		CONTAINER_OF(cb, struct stts751_data, gpio_cb);
	const struct stts751_config *cfg = stts751->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_STTS751_TRIGGER_OWN_THREAD)
	k_sem_give(&stts751->gpio_sem);
#elif defined(CONFIG_STTS751_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&stts751->work);
#endif /* CONFIG_STTS751_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_STTS751_TRIGGER_OWN_THREAD
static void stts751_thread(struct stts751_data *stts751)
{
	while (1) {
		k_sem_take(&stts751->gpio_sem, K_FOREVER);
		stts751_handle_interrupt(stts751->dev);
	}
}
#endif /* CONFIG_STTS751_TRIGGER_OWN_THREAD */

#ifdef CONFIG_STTS751_TRIGGER_GLOBAL_THREAD
static void stts751_work_cb(struct k_work *work)
{
	struct stts751_data *stts751 =
		CONTAINER_OF(work, struct stts751_data, work);

	stts751_handle_interrupt(stts751->dev);
}
#endif /* CONFIG_STTS751_TRIGGER_GLOBAL_THREAD */

int stts751_init_interrupt(const struct device *dev)
{
	struct stts751_data *stts751 = dev->data;
	const struct stts751_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->int_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

#if defined(CONFIG_STTS751_TRIGGER_OWN_THREAD)
	k_sem_init(&stts751->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&stts751->thread, stts751->thread_stack,
			CONFIG_STTS751_THREAD_STACK_SIZE,
			(k_thread_entry_t)stts751_thread, stts751,
			NULL, NULL, K_PRIO_COOP(CONFIG_STTS751_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_STTS751_TRIGGER_GLOBAL_THREAD)
	stts751->work.handler = stts751_work_cb;
#endif /* CONFIG_STTS751_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&stts751->gpio_cb, stts751_gpio_callback, BIT(cfg->int_gpio.pin));

	if (gpio_add_callback(cfg->int_gpio.port, &stts751->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* Enable interrupt on high temperature */
	float temp_hi = (float) CONFIG_STTS751_TEMP_HI_THRESHOLD;
	float temp_lo = (float) CONFIG_STTS751_TEMP_LO_THRESHOLD;

	stts751_high_temperature_threshold_set(stts751->ctx,
					stts751_from_celsius_to_lsb(temp_hi));

	stts751_low_temperature_threshold_set(stts751->ctx,
					stts751_from_celsius_to_lsb(temp_lo));

	return gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
