/*
 * Original license from ina23x driver:
 *  Copyright 2021 The Chromium OS Authors
 *  Copyright 2021 Grinn
 *
 * Copyright 2024, Remie Lowik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina700.h"
#include "ina700_trigger.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(INA700_TRIGGER, CONFIG_SENSOR_LOG_LEVEL);

static void ina700_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pin)
{
	ARG_UNUSED(pin);

	struct ina700_trigger *trigg = CONTAINER_OF(cb, struct ina700_trigger, gpio_cb);

	k_work_submit(&trigg->conversion_work);
}

int ina700_trigger_mode_init(struct ina700_trigger *trigg, const struct gpio_dt_spec *alert_gpio)
{
	int ret;

	if (!device_is_ready(alert_gpio->port)) {
		LOG_ERR("Alert GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(alert_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&trigg->gpio_cb, ina700_gpio_callback, BIT(alert_gpio->pin));

	ret = gpio_add_callback(alert_gpio->port, &trigg->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(alert_gpio, GPIO_INT_EDGE_FALLING);
}
