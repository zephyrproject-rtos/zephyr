/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm6001_gpio

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/gpio/nordic-npm6001-gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

/* nPM6001 GPIO related registers */
#define NPM6001_GPIOOUTSET 0x69U
#define NPM6001_GPIOOUTCLR 0x6AU
#define NPM6001_GPIOIN	   0x6BU
#define NPM6001_GPIO0CONF  0x6CU
#define NPM6001_GPIO1CONF  0x6DU
#define NPM6001_GPIO2CONF  0x6EU

/* GPIO(0|1|2)CONF fields */
#define NPM6001_CONF_DIRECTION_OUTPUT BIT(0)
#define NPM6001_CONF_INPUT_ENABLED    BIT(1)
#define NPM6001_CONF_PULLDOWN_ENABLED BIT(2)
#define NPM6001_CONF_DRIVE_HIGH	      BIT(5)
#define NPM6001_CONF_SENSE_CMOS	      BIT(6)

#define NPM6001_PIN_MAX 2U
#define NPM6001_PIN_MSK 0x7U

struct gpio_npm6001_config {
	struct gpio_driver_config common;
	struct i2c_dt_spec bus;
};

struct gpio_npm6001_data {
	struct gpio_driver_data common;
};

static int gpio_npm6001_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_npm6001_config *config = dev->config;
	uint8_t reg = NPM6001_GPIOIN;
	uint8_t val;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = i2c_write_read_dt(&config->bus, &reg, sizeof(reg), &val,
				sizeof(val));
	if (ret < 0) {
		return ret;
	}

	*value = val;

	return 0;
}

static int gpio_npm6001_port_set_bits_raw(const struct device *dev,
					  gpio_port_pins_t pins)
{
	const struct gpio_npm6001_config *config = dev->config;
	uint8_t buf[2] = {NPM6001_GPIOOUTSET, (uint8_t)pins & NPM6001_PIN_MSK};

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	return i2c_write_dt(&config->bus, buf, sizeof(buf));
}

static int gpio_npm6001_port_clear_bits_raw(const struct device *dev,
					    gpio_port_pins_t pins)
{
	const struct gpio_npm6001_config *config = dev->config;
	uint8_t buf[2] = {NPM6001_GPIOOUTCLR, (uint8_t)pins & NPM6001_PIN_MSK};

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	return i2c_write_dt(&config->bus, buf, sizeof(buf));
}

static inline int gpio_npm6001_configure(const struct device *dev,
					 gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_npm6001_config *config = dev->config;
	uint8_t buf[2] = {NPM6001_GPIO0CONF, 0U};

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pin > NPM6001_PIN_MAX) {
		return -EINVAL;
	}

	/* select GPIO0CONF/GPIO1CONF/GPIO2CONF */
	buf[0] += pin;

	if ((flags & GPIO_OUTPUT) != 0U) {
		/* input buffer enabled to allow reading output level */
		buf[1] |= NPM6001_CONF_DIRECTION_OUTPUT |
			  NPM6001_CONF_INPUT_ENABLED;

		/* open-drain/open-source not supported */
		if ((flags & GPIO_SINGLE_ENDED) != 0U) {
			return -ENOTSUP;
		}

		/* drive strength (defaults to normal) */
		if ((flags & NPM6001_GPIO_DRIVE_MSK) ==
		    NPM6001_GPIO_DRIVE_HIGH) {
			buf[1] |= NPM6001_CONF_DRIVE_HIGH;
		}

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			int ret;

			ret = gpio_npm6001_port_set_bits_raw(
				dev, (gpio_port_pins_t)BIT(pin));
			if (ret < 0) {
				return ret;
			}
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			int ret;

			ret = gpio_npm6001_port_clear_bits_raw(
				dev, (gpio_port_pins_t)BIT(pin));
			if (ret < 0) {
				return ret;
			}
		}
	} else if ((flags & GPIO_INPUT) != 0U) {
		buf[1] |= NPM6001_CONF_INPUT_ENABLED;

		/* pull resistor */
		if ((flags & GPIO_PULL_DOWN) != 0U) {
			buf[1] |= NPM6001_CONF_PULLDOWN_ENABLED;
		} else if ((flags & GPIO_PULL_UP) != 0U) {
			return -ENOTSUP;
		}

		/* input type (defaults to schmitt trigger) */
		if ((flags & NPM6001_GPIO_SENSE_MSK) ==
		    NPM6001_GPIO_SENSE_CMOS) {
			buf[1] |= NPM6001_CONF_SENSE_CMOS;
		}
	} else {
		return -ENOTSUP;
	}

	return i2c_write_dt(&config->bus, buf, sizeof(buf));
}

static int gpio_npm6001_port_set_masked_raw(const struct device *dev,
					    gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	int ret;

	ret = gpio_npm6001_port_set_bits_raw(dev, mask & value);
	if (ret < 0) {
		return ret;
	}

	return gpio_npm6001_port_clear_bits_raw(
		dev, mask & (~value & NPM6001_PIN_MSK));
}

static int gpio_npm6001_port_toggle_bits(const struct device *dev,
					 gpio_port_pins_t pins)
{
	uint32_t val;
	int ret;

	ret = gpio_npm6001_port_get_raw(dev, &val);
	if (ret < 0) {
		return ret;
	}

	return gpio_npm6001_port_set_masked_raw(dev, pins,
						~val & NPM6001_PIN_MSK);
}

static const struct gpio_driver_api gpio_npm6001_api = {
	.pin_configure = gpio_npm6001_configure,
	.port_get_raw = gpio_npm6001_port_get_raw,
	.port_set_masked_raw = gpio_npm6001_port_set_masked_raw,
	.port_set_bits_raw = gpio_npm6001_port_set_bits_raw,
	.port_clear_bits_raw = gpio_npm6001_port_clear_bits_raw,
	.port_toggle_bits = gpio_npm6001_port_toggle_bits,
};

static int gpio_npm6001_init(const struct device *dev)
{
	const struct gpio_npm6001_config *config = dev->config;

	if (!device_is_ready(config->bus.bus)) {
		return -ENODEV;
	}

	return 0;
}

#define GPIO_NPM6001_DEFINE(n)                                                 \
	static const struct gpio_npm6001_config gpio_npm6001_config##n = {     \
		.common =                                                      \
			{                                                      \
				.port_pin_mask =                               \
					GPIO_PORT_PIN_MASK_FROM_DT_INST(n),    \
			},                                                     \
		.bus = I2C_DT_SPEC_GET(DT_INST_PARENT(n))};                    \
                                                                               \
	static struct gpio_npm6001_data gpio_npm6001_data##n;                  \
                                                                               \
	DEVICE_DT_INST_DEFINE(n, &gpio_npm6001_init, NULL,                     \
			      &gpio_npm6001_data##n, &gpio_npm6001_config##n,  \
			      POST_KERNEL, CONFIG_GPIO_NPM6001_INIT_PRIORITY,  \
			      &gpio_npm6001_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NPM6001_DEFINE)
