/*
 * Copyright 2021 The Chromium OS Authors
 * Copyright 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina230.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(INA230, CONFIG_SENSOR_LOG_LEVEL);

static void ina230_work_handler(struct k_work *work)
{
	struct ina230_data *ina230 = CONTAINER_OF(work, struct ina230_data, work);

	if (ina230->handler_alert != NULL) {
		ina230->handler_alert(ina230->dev, ina230->trig_alert);
	}
}

static void ina230_gpio_callback(const struct device *port,
				 struct gpio_callback *cb, uint32_t pin)
{
	struct ina230_data *ina230 = CONTAINER_OF(cb, struct ina230_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

	k_work_submit(&ina230->work);
}

int ina230_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct ina230_data *ina230 = dev->data;

	ARG_UNUSED(trig);

	ina230->handler_alert = handler;
	ina230->trig_alert = trig;

	return 0;
}

int ina230_trigger_mode_init(const struct device *dev)
{
	struct ina230_data *ina230 = dev->data;
	const struct ina230_config *config = dev->config;
	int ret;

	/* setup alert gpio interrupt */
	if (!gpio_is_ready_dt(&config->alert_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(config->alert_gpio.port);
		return -ENODEV;
	}

	ina230->dev = dev;
	k_work_init(&ina230->work, ina230_work_handler);

	ret = gpio_pin_configure_dt(&config->alert_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&ina230->gpio_cb,
			   ina230_gpio_callback,
			   BIT(config->alert_gpio.pin));

	ret = gpio_add_callback(config->alert_gpio.port, &ina230->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&config->alert_gpio,
					       GPIO_INT_EDGE_BOTH);
}
