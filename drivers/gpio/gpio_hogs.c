/*
 * Copyright (c) 2022-2023 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_hogs, CONFIG_GPIO_LOG_LEVEL);

int gpio_hogs_init(const struct device *dev)
{
	const struct gpio_driver_config *cfg = dev->config;
	const struct gpio_hog_dt_spec *spec;
	int err;

	for (size_t i = 0; i < cfg->gpio_hogs.num_specs; i++) {
		spec = &cfg->gpio_hogs.specs[i];

		err = gpio_pin_configure(dev, spec->pin, spec->flags);
		if (err < 0) {
			LOG_ERR("failed to configure GPIO hog for port %s pin %u (err %d)",
				dev->name, spec->pin, err);
			return err;
		}
	}

	return 0;
}
