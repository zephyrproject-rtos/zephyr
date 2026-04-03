/*
 * Copyright 2022 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina2xx_trigger.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(INA2XX, CONFIG_SENSOR_LOG_LEVEL);

static void ina2xx_gpio_callback(const struct device *port,
				 struct gpio_callback *cb, uint32_t pin)
{
	ARG_UNUSED(pin);

	struct ina2xx_trigger *trigg = CONTAINER_OF(cb, struct ina2xx_trigger, gpio_cb);

	k_work_submit(&trigg->conversion_work);
}

int ina2xx_trigger_mode_init(struct ina2xx_trigger *trigg, const struct gpio_dt_spec *alert_gpio)
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

	gpio_init_callback(&trigg->gpio_cb,
			   ina2xx_gpio_callback,
			   BIT(alert_gpio->pin));

	ret = gpio_add_callback(alert_gpio->port, &trigg->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(alert_gpio,
					       GPIO_INT_EDGE_FALLING);
}
