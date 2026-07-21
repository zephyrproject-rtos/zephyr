/*
 * Copyright 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT zephyr_charger_gpio

struct charger_gpio_config {
	struct gpio_dt_spec ctrl_gpio;
};

static int charger_gpio_get_prop(const struct device *dev, charger_prop_t prop,
				 union charger_propval *val)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(prop);
	ARG_UNUSED(val);

	return -ENOTSUP;
}

static int charger_gpio_set_prop(const struct device *dev, charger_prop_t prop,
				 const union charger_propval *val)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(prop);
	ARG_UNUSED(val);

	return -ENOTSUP;
}

static int charger_gpio_enable(const struct device *dev, const bool enable)
{
	const struct charger_gpio_config *cfg = dev->config;

	return gpio_pin_set_dt(&cfg->ctrl_gpio, enable);
}

static int charger_gpio_init(const struct device *dev)
{
	const struct charger_gpio_config *cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->ctrl_gpio)) {
		return -ENODEV;
	}

	return gpio_pin_configure_dt(&cfg->ctrl_gpio, GPIO_OUTPUT_ACTIVE);
}

static DEVICE_API(charger, charger_gpio_api) = {
	.get_property = charger_gpio_get_prop,
	.set_property = charger_gpio_set_prop,
	.charge_enable = charger_gpio_enable,
};

#define CHARGER_GPIO_INIT(inst)                                                                    \
	static const struct charger_gpio_config charger_gpio##_config_##inst = {                   \
		.ctrl_gpio = GPIO_DT_SPEC_INST_GET(inst, ctrl_gpios),                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, charger_gpio_init, NULL, NULL, &charger_gpio##_config_##inst,  \
			      POST_KERNEL, CONFIG_CHARGER_INIT_PRIORITY, &charger_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(CHARGER_GPIO_INIT)
