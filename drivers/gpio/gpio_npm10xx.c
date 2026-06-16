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

LOG_MODULE_REGISTER(gpio_npm10xx, CONFIG_GPIO_LOG_LEVEL);

#define NPM10XX_GPIO_PINS 3U

/* Register Offsets */
#define NPM10_GPIO_CONFIG0 0xA0U
#define NPM10_GPIO_OUTPUT0 0xA6U
#define NPM10_GPIO_READ    0xACU

/* CONFIGx (0xA0–0xA2) */
#define GPIO_CONFIG_INPUT     BIT(0)
#define GPIO_CONFIG_OUTPUT    BIT(1)
#define GPIO_CONFIG_OPENDRAIN BIT(2)
#define GPIO_CONFIG_PULLDOWN  BIT(3)
#define GPIO_CONFIG_PULLUP    BIT(4)
#define GPIO_CONFIG_DRIVE     BIT(5)
#define GPIO_CONFIG_DEBOUNCE  BIT(6)

/* USAGEx (0xA3–0xA5) */
#define GPIO_USAGE_POL_INVERT BIT(4)

struct gpio_npm10xx_config {
	struct gpio_driver_config common;
	struct i2c_dt_spec i2c;
};

struct gpio_npm10xx_data {
	struct gpio_driver_data common;
};

int gpio_npm10xx_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_npm10xx_config *config = dev->config;
	uint8_t conf;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pin >= NPM10XX_GPIO_PINS) {
		LOG_ERR("pin number out of range %d", pin);
		return -EINVAL;
	}

	conf = FIELD_PREP(GPIO_CONFIG_INPUT, !!(flags & GPIO_INPUT)) |
	       FIELD_PREP(GPIO_CONFIG_OUTPUT, !!(flags & GPIO_OUTPUT)) |
	       FIELD_PREP(GPIO_CONFIG_OPENDRAIN, (flags & GPIO_OPEN_DRAIN) == GPIO_OPEN_DRAIN) |
	       FIELD_PREP(GPIO_CONFIG_PULLDOWN, !!(flags & GPIO_PULL_DOWN)) |
	       FIELD_PREP(GPIO_CONFIG_PULLUP, !!(flags & GPIO_PULL_UP)) |
	       FIELD_PREP(GPIO_CONFIG_DRIVE, !!(flags & NPM10XX_GPIO_DRIVE_HIGH)) |
	       FIELD_PREP(GPIO_CONFIG_DEBOUNCE, !!(flags & NPM10XX_GPIO_DEBOUNCE_ON));

	return i2c_reg_write_byte_dt(&config->i2c, NPM10_GPIO_CONFIG0 + pin, conf);
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

	for (size_t i = 0; i < NPM10XX_GPIO_PINS; i++) {
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
		.i2c = I2C_DT_SPEC_GET(DT_INST_PARENT(n)),                                         \
	};                                                                                         \
                                                                                                   \
	static struct gpio_npm10xx_data gpio_npm10xx_data##n;                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_npm10xx_init, NULL, &gpio_npm10xx_data##n,                   \
			      &gpio_npm10xx_config##n, POST_KERNEL,                                \
			      CONFIG_GPIO_NPM10XX_INIT_PRIORITY, &gpio_npm10xx_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NPM10XX_DEFINE)
