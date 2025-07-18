/* TI TMP11X temperature sensor trigger support
 *
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp11x

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "tmp11x.h"

LOG_MODULE_DECLARE(TMP11X, CONFIG_SENSOR_LOG_LEVEL);

/**
 * tmp11x_trigger_set - link external trigger to threshold event
 */
int tmp11x_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct tmp11x_data *tmp11x = dev->data;
	const struct tmp11x_dev_config *config = dev->config;

	if (!config->alert_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->chan != SENSOR_CHAN_ALL &&
	    trig->chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Unsupported sensor trigger channel %d", trig->chan);
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		LOG_ERR("Unsupported sensor trigger type %d", trig->type);
		return -ENOTSUP;
	}

	tmp11x->alert_handler = handler;
	tmp11x->alert_trigger = trig;

	return 0;
}

/**
 * tmp11x_handle_interrupt - handle the alert event
 * read status and call handler if registered
 */
static void tmp11x_handle_interrupt(const struct device *dev)
{
	struct tmp11x_data *tmp11x = dev->data;
	const struct tmp11x_dev_config *cfg = dev->config;
	uint16_t config_reg;
	int ret;

	/* Read configuration register to check alert status */
	ret = tmp11x_reg_read(dev, TMP11X_REG_CFGR, &config_reg);
	if (ret < 0) {
		LOG_ERR("Failed to read config register: %d", ret);
		goto re_enable;
	}

	/* Call the user's alert handler if registered */
	if (tmp11x->alert_handler != NULL) {
		tmp11x->alert_handler(dev, tmp11x->alert_trigger);
	}

re_enable:
	/* Re-enable interrupt - TMP11X alert pin is level-triggered */
	gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_LEVEL_ACTIVE);
}

static void tmp11x_gpio_callback(const struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct tmp11x_data *tmp11x =
		CONTAINER_OF(cb, struct tmp11x_data, alert_cb);
	const struct tmp11x_dev_config *cfg = tmp11x->dev->config;

	ARG_UNUSED(pins);

	/* Disable interrupt to avoid retriggering */
	gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_TMP11X_TRIGGER_OWN_THREAD)
	k_sem_give(&tmp11x->gpio_sem);
#elif defined(CONFIG_TMP11X_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&tmp11x->work);
#endif /* CONFIG_TMP11X_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_TMP11X_TRIGGER_OWN_THREAD
static void tmp11x_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct tmp11x_data *tmp11x = p1;

	while (1) {
		k_sem_take(&tmp11x->gpio_sem, K_FOREVER);
		tmp11x_handle_interrupt(tmp11x->dev);
	}
}
#endif /* CONFIG_TMP11X_TRIGGER_OWN_THREAD */

#ifdef CONFIG_TMP11X_TRIGGER_GLOBAL_THREAD
static void tmp11x_work_cb(struct k_work *work)
{
	struct tmp11x_data *tmp11x =
		CONTAINER_OF(work, struct tmp11x_data, work);

	tmp11x_handle_interrupt(tmp11x->dev);
}
#endif /* CONFIG_TMP11X_TRIGGER_GLOBAL_THREAD */

int tmp11x_init_interrupt(const struct device *dev)
{
	struct tmp11x_data *tmp11x = dev->data;
	const struct tmp11x_dev_config *cfg = dev->config;
	int ret;

	/* Check if alert GPIO is configured in device tree */
	if (!cfg->alert_gpio.port) {
		LOG_DBG("%s: Alert GPIO not configured", dev->name);
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->alert_gpio)) {
		LOG_ERR("%s: Alert GPIO controller not ready", dev->name);
		return -ENODEV;
	}

#if defined(CONFIG_TMP11X_TRIGGER_OWN_THREAD)
	k_sem_init(&tmp11x->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&tmp11x->thread, tmp11x->thread_stack,
			CONFIG_TMP11X_THREAD_STACK_SIZE,
			tmp11x_thread, tmp11x,
			NULL, NULL, K_PRIO_COOP(CONFIG_TMP11X_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&tmp11x->thread, dev->name);
#elif defined(CONFIG_TMP11X_TRIGGER_GLOBAL_THREAD)
	tmp11x->work.handler = tmp11x_work_cb;
#endif /* CONFIG_TMP11X_TRIGGER_OWN_THREAD */

	/* Configure GPIO as input */
	ret = gpio_pin_configure_dt(&cfg->alert_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("%s: Failed to configure alert GPIO", dev->name);
		return ret;
	}

	/* Initialize GPIO callback */
	gpio_init_callback(&tmp11x->alert_cb, tmp11x_gpio_callback,
			   BIT(cfg->alert_gpio.pin));

	/* Add callback to GPIO controller */
	ret = gpio_add_callback(cfg->alert_gpio.port, &tmp11x->alert_cb);
	if (ret < 0) {
		LOG_ERR("%s: Failed to add alert GPIO callback", dev->name);
		return ret;
	}

	/* Set default alert pin configuration: active low, therm mode */
	ret = tmp11x_write_config(dev, TMP11X_CFGR_ALERT_PIN_POL | TMP11X_CFGR_ALERT_PIN_MODE, 0);
	if (ret < 0) {
		LOG_ERR("%s: Failed to configure alert pin", dev->name);
		return ret;
	}

	/* Enable interrupt - TMP11X alert pin is level-triggered */
	ret = gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_LEVEL_ACTIVE);
	if (ret < 0) {
		LOG_ERR("%s: Failed to configure alert pin interrupt", dev->name);
		return ret;
	}

	LOG_DBG("%s: Alert pin initialized successfully", dev->name);
	return 0;
}
