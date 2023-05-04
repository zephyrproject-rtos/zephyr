/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO driver for the ADS114S0x AFE.
 */

#define DT_DRV_COMPAT ti_ads114s0x_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_ads114s0x);

#include <zephyr/drivers/adc/ads114s0x.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

struct gpio_ads114s0x_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	const struct device *parent;
};

struct gpio_ads114s0x_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
};

static int gpio_ads114s0x_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_ads114s0x_config *config = dev->config;
	int err = 0;

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		return ads114s0x_gpio_deconfigure(config->parent, pin);
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if (flags & GPIO_INT_ENABLE) {
		/* ads114s0x GPIOs do not support interrupts */
		return -ENOTSUP;
	}

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_INPUT:
		err = ads114s0x_gpio_set_input(config->parent, pin);
		break;
	case GPIO_OUTPUT:
		err = ads114s0x_gpio_set_output(config->parent, pin,
						(flags & GPIO_OUTPUT_INIT_HIGH) != 0);
		break;
	default:
		return -ENOTSUP;
	}

	return err;
}

static int gpio_ads114s0x_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_ads114s0x_config *config = dev->config;

	return ads114s0x_gpio_port_get_raw(config->parent, value);
}

static int gpio_ads114s0x_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					      gpio_port_value_t value)
{
	const struct gpio_ads114s0x_config *config = dev->config;

	return ads114s0x_gpio_port_set_masked_raw(config->parent, mask, value);
}

static int gpio_ads114s0x_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ads114s0x_config *config = dev->config;

	return ads114s0x_gpio_port_set_masked_raw(config->parent, pins, pins);
}

static int gpio_ads114s0x_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ads114s0x_config *config = dev->config;

	return ads114s0x_gpio_port_set_masked_raw(config->parent, pins, 0);
}

static int gpio_ads114s0x_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ads114s0x_config *config = dev->config;

	return ads114s0x_gpio_port_toggle_bits(config->parent, pins);
}

static int gpio_ads114s0x_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						  enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(mode);
	ARG_UNUSED(trig);

	return -ENOTSUP;
}

static int gpio_ads114s0x_init(const struct device *dev)
{
	const struct gpio_ads114s0x_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("parent ads114s0x device '%s' not ready", config->parent->name);
		return -EINVAL;
	}

	return 0;
}

static const struct gpio_driver_api gpio_ads114s0x_api = {
	.pin_configure = gpio_ads114s0x_config,
	.port_set_masked_raw = gpio_ads114s0x_port_set_masked_raw,
	.port_set_bits_raw = gpio_ads114s0x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ads114s0x_port_clear_bits_raw,
	.port_toggle_bits = gpio_ads114s0x_port_toggle_bits,
	.pin_interrupt_configure = gpio_ads114s0x_pin_interrupt_configure,
	.port_get_raw = gpio_ads114s0x_port_get_raw,
};

BUILD_ASSERT(CONFIG_GPIO_ADS114S0X_INIT_PRIORITY > CONFIG_ADC_INIT_PRIORITY,
	     "ADS114S0X GPIO driver must be initialized after ADS114S0X ADC driver");

#define GPIO_ADS114S0X_DEVICE(id)                                                                  \
	static const struct gpio_ads114s0x_config gpio_ads114s0x_##id##_cfg = {                    \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(id)},                  \
		.parent = DEVICE_DT_GET(DT_INST_BUS(id)),                                          \
	};                                                                                         \
                                                                                                   \
	static struct gpio_ads114s0x_data gpio_ads114s0x_##id##_data;                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &gpio_ads114s0x_init, NULL, &gpio_ads114s0x_##id##_data,         \
			      &gpio_ads114s0x_##id##_cfg, POST_KERNEL,                             \
			      CONFIG_GPIO_ADS114S0X_INIT_PRIORITY, &gpio_ads114s0x_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_ADS114S0X_DEVICE)
