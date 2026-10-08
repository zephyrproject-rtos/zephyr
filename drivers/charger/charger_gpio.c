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
	struct gpio_dt_spec charging_gpio;
	struct gpio_dt_spec charge_done_gpio;
};

static int charger_gpio_get_prop(const struct device *dev, charger_prop_t prop,
				 union charger_propval *val)
{
	const struct charger_gpio_config *cfg = dev->config;
	int charging;
	int charge_done = 0;

	if ((prop != CHARGER_PROP_STATUS) || (cfg->charging_gpio.port == NULL)) {
		return -ENOTSUP;
	}

	charging = gpio_pin_get_dt(&cfg->charging_gpio);
	if (charging < 0) {
		return charging;
	}

	if (cfg->charge_done_gpio.port != NULL) {
		charge_done = gpio_pin_get_dt(&cfg->charge_done_gpio);
		if (charge_done < 0) {
			return charge_done;
		}
	}

	if ((charging > 0) && (charge_done > 0)) {
		val->status = CHARGER_STATUS_UNKNOWN;
	} else if (charging > 0) {
		val->status = CHARGER_STATUS_CHARGING;
	} else if (charge_done > 0) {
		val->status = CHARGER_STATUS_FULL;
	} else {
		val->status = CHARGER_STATUS_NOT_CHARGING;
	}

	return 0;
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

	if (cfg->ctrl_gpio.port == NULL) {
		return -ENOTSUP;
	}

	return gpio_pin_set_dt(&cfg->ctrl_gpio, enable);
}

static int charger_gpio_configure(const struct gpio_dt_spec *gpio, gpio_flags_t flags)
{
	if (gpio->port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(gpio)) {
		return -ENODEV;
	}

	return gpio_pin_configure_dt(gpio, flags);
}

static int charger_gpio_init(const struct device *dev)
{
	const struct charger_gpio_config *cfg = dev->config;
	int ret;

	ret = charger_gpio_configure(&cfg->charging_gpio, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	ret = charger_gpio_configure(&cfg->charge_done_gpio, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	return charger_gpio_configure(&cfg->ctrl_gpio, GPIO_OUTPUT_ACTIVE);
}

static DEVICE_API(charger, charger_gpio_api) = {
	.get_property = charger_gpio_get_prop,
	.set_property = charger_gpio_set_prop,
	.charge_enable = charger_gpio_enable,
};

#define CHARGER_GPIO_INIT(inst)                                                                    \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, ctrl_gpios) ||                                   \
		     DT_INST_NODE_HAS_PROP(inst, charging_gpios),                                 \
		     "A control or charging status GPIO is required");                            \
	BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(inst, charge_done_gpios) ||                           \
		     DT_INST_NODE_HAS_PROP(inst, charging_gpios),                                 \
		     "charge-done-gpios requires charging-gpios");                                \
	static const struct charger_gpio_config charger_gpio##_config_##inst = {                   \
		.ctrl_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, ctrl_gpios, {0}),                     \
		.charging_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, charging_gpios, {0}),             \
		.charge_done_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, charge_done_gpios, {0}),       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, charger_gpio_init, NULL, NULL, &charger_gpio##_config_##inst,  \
			      POST_KERNEL, CONFIG_CHARGER_INIT_PRIORITY, &charger_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(CHARGER_GPIO_INIT)
