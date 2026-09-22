/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm10xx_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/gpio/nordic-npm10xx-gpio.h>
#include <zephyr/logging/log.h>

#include "mfd_npm10xx.h"

LOG_MODULE_REGISTER(gpio_npm10xx, CONFIG_GPIO_LOG_LEVEL);

/* Register Offsets */
#define NPM10_GPIO_OUTPUT0 0xA6U
#define NPM10_GPIO_READ    0xACU

struct gpio_npm10xx_config {
	struct gpio_driver_config common;
	const struct device *mfd;
	struct i2c_dt_spec i2c;
};

struct gpio_npm10xx_data {
	struct gpio_driver_data common;
};

int gpio_npm10xx_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				     gpio_port_value_t value);

int gpio_npm10xx_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_npm10xx_config *config = dev->config;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = mfd_npm10xx_pin_configure(config->mfd, pin, NPM10_PIN_GPIO, flags);
	if (ret < 0) {
		LOG_ERR("Failed to configure pin %u for GPIO usage", pin);
		return ret;
	}

	if (flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH)) {
		return gpio_npm10xx_port_set_masked_raw(
			dev, BIT(pin), flags & GPIO_OUTPUT_INIT_HIGH ? BIT(pin) : 0U);
	}

	return 0;
}

int gpio_npm10xx_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_npm10xx_config *config = dev->config;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	return i2c_reg_read_byte_dt(&config->i2c, NPM10_GPIO_READ, (uint8_t *)value);
}

int gpio_npm10xx_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				     gpio_port_value_t value)
{
	const struct gpio_npm10xx_config *config = dev->config;
	int ret;

	for (size_t i = 0; i < NPM10_PIN_NUM; i++) {
		if (IS_BIT_SET(mask, i)) {
			ret = i2c_reg_write_byte_dt(&config->i2c, NPM10_GPIO_OUTPUT0 + i,
						    IS_BIT_SET(value, i));
			if (ret < 0) {
				return ret;
			}
		}
	}

	return 0;
}

int gpio_npm10xx_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_npm10xx_port_set_masked_raw(dev, pins, pins);
}

int gpio_npm10xx_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_npm10xx_port_set_masked_raw(dev, pins, 0U);
}

int gpio_npm10xx_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	gpio_port_pins_t value;

	ret = gpio_npm10xx_port_get_raw(dev, &value);
	if (ret < 0) {
		return ret;
	}

	return gpio_npm10xx_port_set_masked_raw(dev, pins, ~value);
}

int gpio_npm10xx_init(const struct device *dev)
{
	const struct gpio_npm10xx_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(gpio, gpio_npm10xx_api) = {
	.pin_configure = gpio_npm10xx_pin_configure,
	.port_get_raw = gpio_npm10xx_port_get_raw,
	.port_set_masked_raw = gpio_npm10xx_port_set_masked_raw,
	.port_set_bits_raw = gpio_npm10xx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_npm10xx_port_clear_bits_raw,
	.port_toggle_bits = gpio_npm10xx_port_toggle_bits,
};

#define GPIO_NPM10XX_DEFINE(n)                                                                     \
	static const struct gpio_npm10xx_config gpio_npm10xx_config##n = {                         \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(n)),                                         \
	};                                                                                         \
                                                                                                   \
	static struct gpio_npm10xx_data gpio_npm10xx_data##n;                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_npm10xx_init, NULL, &gpio_npm10xx_data##n,                   \
			      &gpio_npm10xx_config##n, POST_KERNEL,                                \
			      CONFIG_GPIO_NPM10XX_INIT_PRIORITY, &gpio_npm10xx_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NPM10XX_DEFINE)
