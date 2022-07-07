/*
 * Copyright 2021 The Chromium OS Authors
 * Copyright 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "ina230.h"

LOG_MODULE_DECLARE(INA230, CONFIG_SENSOR_LOG_LEVEL);

static void ina230_gpio_callback(const struct device *port,
				 struct gpio_callback *cb, uint32_t pin)
{
	struct sensor_trigger ina230_trigger;
	struct ina230_data *ina230 = CONTAINER_OF(cb, struct ina230_data, gpio_cb);
	const struct device *dev = (const struct device *)ina230->dev;

	ARG_UNUSED(port);
	ARG_UNUSED(pin);
	ARG_UNUSED(cb);

	if (ina230->handler_alert) {
		ina230_trigger.type = SENSOR_TRIG_DATA_READY;
		ina230->handler_alert(dev, &ina230_trigger);
	}
}

int ina230_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct ina230_data *ina230 = dev->data;

	ARG_UNUSED(trig);

	ina230->handler_alert = handler;

	return 0;
}

int ina230_trigger_mode_init(const struct device *dev)
{
	struct ina230_data *ina230 = dev->data;
	const struct ina230_config *config = dev->config;
	int ret;

	/* setup alert gpio interrupt */
	if (!device_is_ready(config->gpio_alert.port)) {
		LOG_ERR("Alert GPIO device not ready");
		return -ENODEV;
	}

	ina230->dev = dev;

	ret = gpio_pin_configure_dt(&config->gpio_alert, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&ina230->gpio_cb,
			   ina230_gpio_callback,
			   BIT(config->gpio_alert.pin));

	ret = gpio_add_callback(config->gpio_alert.port, &ina230->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&config->gpio_alert,
					       GPIO_INT_EDGE_BOTH);
}
