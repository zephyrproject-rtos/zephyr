/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "ina23x.h"

LOG_MODULE_DECLARE(INA23X, CONFIG_SENSOR_LOG_LEVEL);

static void ina23x_gpio_callback(const struct device *port,
				 struct gpio_callback *cb, uint32_t pin)
{
	struct sensor_trigger ina23x_trigger;
	struct ina23x_data *ina23x = CONTAINER_OF(cb, struct ina23x_data, gpio_cb);
	const struct device *dev = (const struct device *)ina23x->dev;

	ARG_UNUSED(port);
	ARG_UNUSED(pin);
	ARG_UNUSED(cb);

	if (ina23x->handler_alert) {
		ina23x_trigger.type = SENSOR_TRIG_DATA_READY;
		ina23x->handler_alert(dev, &ina23x_trigger);
	}
}

int ina23x_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct ina23x_data *ina23x = dev->data;

	ARG_UNUSED(trig);

	ina23x->handler_alert = handler;

	return 0;
}

int ina23x_trigger_mode_init(const struct device *dev)
{
	struct ina23x_data *ina23x = dev->data;
	const struct ina23x_config *config = dev->config;
	int ret;

	/* setup alert gpio interrupt */
	if (!device_is_ready(config->gpio_alert.port)) {
		LOG_ERR("Alert GPIO device not ready");
		return -ENODEV;
	}

	ina23x->dev = dev;

	ret = gpio_pin_configure_dt(&config->gpio_alert, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&ina23x->gpio_cb,
			   ina23x_gpio_callback,
			   BIT(config->gpio_alert.pin));

	ret = gpio_add_callback(config->gpio_alert.port, &ina23x->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&config->gpio_alert,
					       GPIO_INT_EDGE_BOTH);
}
