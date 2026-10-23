/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cirrus_cs47l63_gpio

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/logging/log.h>

#include "cs47l63.h"

LOG_MODULE_REGISTER(gpio_cs47l63, CONFIG_GPIO_LOG_LEVEL);

BUILD_ASSERT(CONFIG_GPIO_CS47L63_INIT_PRIORITY > CONFIG_AUDIO_CODEC_INIT_PRIORITY,
	     "The CS47L63 GPIO driver must be initialized after the CS47L63 codec driver");

struct gpio_cs47l63_config {
	struct gpio_driver_config common;
	const struct device *parent;
};

struct gpio_cs47l63_data {
	struct gpio_driver_data common;
};

static int gpio_cs47l63_pin_configure(const struct device *dev, gpio_pin_t pin,
				      gpio_flags_t flags)
{
	const struct gpio_cs47l63_config *config = dev->config;

	return cs47l63_gpio_pin_configure(config->parent, pin, flags);
}

static int gpio_cs47l63_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_cs47l63_config *config = dev->config;

	return cs47l63_gpio_port_get_raw(config->parent, value);
}

static int gpio_cs47l63_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_cs47l63_config *config = dev->config;

	return cs47l63_gpio_port_set_masked_raw(config->parent, mask, value);
}

static int gpio_cs47l63_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_cs47l63_port_set_masked_raw(dev, pins, pins);
}

static int gpio_cs47l63_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_cs47l63_port_set_masked_raw(dev, pins, 0U);
}

static int gpio_cs47l63_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_cs47l63_config *config = dev->config;

	return cs47l63_gpio_port_toggle_bits(config->parent, pins);
}

static int gpio_cs47l63_init(const struct device *dev)
{
	const struct gpio_cs47l63_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("Parent codec %s not ready", config->parent->name);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(gpio, gpio_cs47l63_api) = {
	.pin_configure = gpio_cs47l63_pin_configure,
	.port_get_raw = gpio_cs47l63_port_get_raw,
	.port_set_masked_raw = gpio_cs47l63_port_set_masked_raw,
	.port_set_bits_raw = gpio_cs47l63_port_set_bits_raw,
	.port_clear_bits_raw = gpio_cs47l63_port_clear_bits_raw,
	.port_toggle_bits = gpio_cs47l63_port_toggle_bits,
};

#define GPIO_CS47L63_DEFINE(inst)                                                                  \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_INST_PARENT(inst), cirrus_cs47l63),                     \
		     "gpio_cs47l63: " DT_NODE_PATH(DT_DRV_INST(inst)) " is not under a cs47l63");  \
	BUILD_ASSERT(DT_INST_PROP(inst, ngpios) == CS47L63_GPIO_COUNT,                             \
		     "gpio_cs47l63: ngpios must be 12");                                           \
	static const struct gpio_cs47l63_config gpio_cs47l63_##inst##_config = {                   \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
	};                                                                                         \
	static struct gpio_cs47l63_data gpio_cs47l63_##inst##_data;                                \
	DEVICE_DT_INST_DEFINE(inst, gpio_cs47l63_init, NULL, &gpio_cs47l63_##inst##_data,          \
			      &gpio_cs47l63_##inst##_config, POST_KERNEL,                          \
			      CONFIG_GPIO_CS47L63_INIT_PRIORITY, &gpio_cs47l63_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CS47L63_DEFINE)
