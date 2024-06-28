/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT adi_max22017_gpio

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adi_max22017_gpio);

#include <zephyr/drivers/dac/max22017.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

struct gpio_adi_max22017_config {
	const struct device *parent;
};

static int gpio_adi_max22017_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_adi_max22017_config *config = dev->config;
	int err = 0;

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		return adi_max22017_gpio_deconfigure(config->parent, pin);
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_INPUT:
		err = adi_max22017_gpio_set_input(config->parent, pin);
		break;
	case GPIO_OUTPUT:
		err = adi_max22017_gpio_set_output(config->parent, pin,
						   (flags & GPIO_OUTPUT_INIT_HIGH) != 0);
		break;
	default:
		return -ENOTSUP;
	}

	return err;
}

static int gpio_adi_max22017_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						     enum gpio_int_mode mode,
						     enum gpio_int_trig trig)
{
	const struct gpio_adi_max22017_config *config = dev->config;

	return adi_max22017_gpio_pin_interrupt_configure(config->parent, pin, mode, trig);
}

static int gpio_adi_max22017_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_adi_max22017_config *config = dev->config;

	return adi_max22017_gpio_port_get_raw(config->parent, value);
}

static int gpio_adi_max22017_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
						 gpio_port_value_t value)
{
	const struct gpio_adi_max22017_config *config = dev->config;

	return adi_max22017_gpio_port_set_masked_raw(config->parent, mask, value);
}

static int gpio_adi_max22017_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_adi_max22017_config *config = dev->config;

	return adi_max22017_gpio_port_set_masked_raw(config->parent, pins, pins);
}

static int gpio_adi_max22017_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_adi_max22017_config *config = dev->config;

	return adi_max22017_gpio_port_set_masked_raw(config->parent, pins, 0);
}

static int gpio_adi_max22017_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_adi_max22017_config *config = dev->config;

	return adi_max22017_gpio_port_toggle_bits(config->parent, pins);
}

static int gpio_adi_max22017_manage_cb(const struct device *dev, struct gpio_callback *callback,
				       bool set)
{
	const struct gpio_adi_max22017_config *config = dev->config;

	return adi_max22017_manage_cb(config->parent, callback, set);
}

static int gpio_adi_max22017_init(const struct device *dev)
{
	const struct gpio_adi_max22017_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("parent adi_max22017 device '%s' not ready", config->parent->name);
		return -EINVAL;
	}

	return 0;
}

static const struct gpio_driver_api gpio_adi_max22017_api = {
	.pin_configure = gpio_adi_max22017_configure,
	.port_set_masked_raw = gpio_adi_max22017_port_set_masked_raw,
	.port_set_bits_raw = gpio_adi_max22017_port_set_bits_raw,
	.port_clear_bits_raw = gpio_adi_max22017_port_clear_bits_raw,
	.port_toggle_bits = gpio_adi_max22017_port_toggle_bits,
	.port_get_raw = gpio_adi_max22017_port_get_raw,
	.pin_interrupt_configure = gpio_adi_max22017_pin_interrupt_configure,
	.manage_callback = gpio_adi_max22017_manage_cb,
};

BUILD_ASSERT(CONFIG_GPIO_MAX22017_INIT_PRIORITY > CONFIG_DAC_MAX22017_INIT_PRIORITY,
	     "MAX22017 GPIO driver must be initialized after MAX22017 DAC driver");

#define GPIO_MAX22017_DEVICE(id)                                                                   \
	static const struct gpio_adi_max22017_config gpio_adi_max22017_##id##_cfg = {              \
		.parent = DEVICE_DT_GET(DT_INST_BUS(id)),                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, gpio_adi_max22017_init, NULL, NULL,                              \
			      &gpio_adi_max22017_##id##_cfg, POST_KERNEL,                          \
			      CONFIG_GPIO_MAX22017_INIT_PRIORITY, &gpio_adi_max22017_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MAX22017_DEVICE)
