/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm2100_gpio

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/gpio/nordic-npm2100-gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#define NPM2100_GPIO_CONFIG 0x80U
#define NPM2100_GPIO_USAGE  0x83U
#define NPM2100_GPIO_OUTPUT 0x86U
#define NPM2100_GPIO_READ   0x89U

#define NPM2100_GPIO_PINS 2U

#define NPM2100_GPIO_CONFIG_INPUT     0x01U
#define NPM2100_GPIO_CONFIG_OUTPUT    0x02U
#define NPM2100_GPIO_CONFIG_OPENDRAIN 0x04U
#define NPM2100_GPIO_CONFIG_PULLDOWN  0x08U
#define NPM2100_GPIO_CONFIG_PULLUP    0x10U
#define NPM2100_GPIO_CONFIG_DRIVE     0x20U
#define NPM2100_GPIO_CONFIG_DEBOUNCE  0x40U

struct gpio_npm2100_config {
	struct gpio_driver_config common;
	struct i2c_dt_spec i2c;
};

struct gpio_npm2100_data {
	struct gpio_driver_data common;
};

static int gpio_npm2100_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_npm2100_config *config = dev->config;
	uint8_t data;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, NPM2100_GPIO_READ, &data);
	if (ret < 0) {
		return ret;
	}

	*value = data;

	return 0;
}

static int gpio_npm2100_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_npm2100_config *config = dev->config;
	int ret = 0;

	for (size_t idx = 0; idx < NPM2100_GPIO_PINS; idx++) {
		if ((mask & BIT(idx)) != 0U) {
			ret = i2c_reg_write_byte_dt(&config->i2c, NPM2100_GPIO_OUTPUT + idx,
						    !!(value & BIT(idx)));
			if (ret != 0U) {
				return ret;
			}
		}
	}

	return ret;
}

static int gpio_npm2100_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_npm2100_port_set_masked_raw(dev, pins, pins);
}

static int gpio_npm2100_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_npm2100_port_set_masked_raw(dev, pins, 0U);
}

static inline int gpio_npm2100_configure(const struct device *dev, gpio_pin_t pin,
					 gpio_flags_t flags)
{
	const struct gpio_npm2100_config *config = dev->config;
	uint8_t reg = 0U;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pin >= NPM2100_GPIO_PINS) {
		return -EINVAL;
	}

	/* Set initial state if defined */
	if ((flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH)) != 0U) {
		int ret = i2c_reg_write_byte_dt(&config->i2c, NPM2100_GPIO_OUTPUT + pin,
						!!(flags & GPIO_OUTPUT_INIT_HIGH));
		if (ret < 0) {
			return ret;
		}
	}

	/* Set pin configuration */
	if ((flags & GPIO_INPUT) != 0U) {
		reg |= NPM2100_GPIO_CONFIG_INPUT;
	}
	if ((flags & GPIO_OUTPUT) != 0U) {
		reg |= NPM2100_GPIO_CONFIG_OUTPUT;
	}
	if ((flags & GPIO_SINGLE_ENDED) != 0U) {
		reg |= NPM2100_GPIO_CONFIG_OPENDRAIN;
	}
	if ((flags & GPIO_PULL_UP) != 0U) {
		reg |= NPM2100_GPIO_CONFIG_PULLUP;
	}
	if ((flags & GPIO_PULL_DOWN) != 0U) {
		reg |= NPM2100_GPIO_CONFIG_PULLDOWN;
	}
	if ((flags & NPM2100_GPIO_DRIVE_HIGH) != 0U) {
		reg |= NPM2100_GPIO_CONFIG_DRIVE;
	}
	if ((flags & NPM2100_GPIO_DEBOUNCE_ON) != 0U) {
		reg |= NPM2100_GPIO_CONFIG_DEBOUNCE;
	}

	return i2c_reg_write_byte_dt(&config->i2c, NPM2100_GPIO_CONFIG + pin, reg);
}

static int gpio_npm2100_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	int ret;
	uint32_t value;

	ret = gpio_npm2100_port_get_raw(dev, &value);
	if (ret < 0) {
		return ret;
	}

	return gpio_npm2100_port_set_masked_raw(dev, pins, ~value);
}

static DEVICE_API(gpio, gpio_npm2100_api) = {
	.pin_configure = gpio_npm2100_configure,
	.port_get_raw = gpio_npm2100_port_get_raw,
	.port_set_masked_raw = gpio_npm2100_port_set_masked_raw,
	.port_set_bits_raw = gpio_npm2100_port_set_bits_raw,
	.port_clear_bits_raw = gpio_npm2100_port_clear_bits_raw,
	.port_toggle_bits = gpio_npm2100_port_toggle_bits,
};

static int gpio_npm2100_init(const struct device *dev)
{
	const struct gpio_npm2100_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	return 0;
}

#define GPIO_NPM2100_DEFINE(n)                                                                     \
	static const struct gpio_npm2100_config gpio_npm2100_config##n = {                         \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(n))};                                        \
                                                                                                   \
	static struct gpio_npm2100_data gpio_npm2100_data##n;                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &gpio_npm2100_init, NULL, &gpio_npm2100_data##n,                  \
			      &gpio_npm2100_config##n, POST_KERNEL,                                \
			      CONFIG_GPIO_NPM2100_INIT_PRIORITY, &gpio_npm2100_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NPM2100_DEFINE)
