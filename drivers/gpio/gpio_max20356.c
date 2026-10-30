/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max20356_gpio

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mfd/max20356.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "mfd_max20356.h"

/* Number of multi-purpose control pins (MPC0..MPC7). */
#define MAX20356_GPIO_PINS 8U

/* Per-pin config register: MPC<n>Cfg at 0x72 + n */
#define MAX20356_GPIO_REG(pin) (MAX20356_REG_MPC0CFG + (pin))

#define MAX20356_GPIO_PIN_MSK  MAX20356_MPC0CFG_MPC0PIN_MSK
#define MAX20356_GPIO_OUT_MSK  MAX20356_MPC0CFG_MPC0OUT_MSK
#define MAX20356_GPIO_OD_MSK   MAX20356_MPC0CFG_MPC0OD_MSK
#define MAX20356_GPIO_HIZB_MSK MAX20356_MPC0CFG_MPC0HIZB_MSK
#define MAX20356_GPIO_PUP_MSK  MAX20356_MPC0CFG_MPC0PUP_MSK

struct gpio_max20356_config {
	struct gpio_driver_config common;
	const struct device *mfd;
};

struct gpio_max20356_data {
	struct gpio_driver_data common;
};

static int gpio_max20356_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_max20356_config *config = dev->config;
	uint8_t mask = MAX20356_GPIO_HIZB_MSK | MAX20356_GPIO_OD_MSK | MAX20356_GPIO_PUP_MSK |
		       MAX20356_GPIO_OUT_MSK;
	uint8_t val = 0U;

	if (pin >= MAX20356_GPIO_PINS) {
		return -EINVAL;
	}

	/* No internal pull-down on the MPC pins. */
	if ((flags & GPIO_PULL_DOWN) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_INPUT) != 0U && (flags & GPIO_OUTPUT) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) != 0U) {
		/* Enable the output driver (Hi-Z-bar) and set the initial level. */
		val |= MAX20356_GPIO_HIZB_MSK;

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			val |= MAX20356_GPIO_OUT_MSK;
		}

		if ((flags & GPIO_SINGLE_ENDED) != 0U) {
			if ((flags & GPIO_LINE_OPEN_DRAIN) == 0U) {
				return -ENOTSUP;
			}

			val |= MAX20356_GPIO_OD_MSK;
		}
	} else if ((flags & GPIO_INPUT) == 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_UP) != 0U) {
		val |= MAX20356_GPIO_PUP_MSK;
	}

	return mfd_max20356_reg_update(config->mfd, MAX20356_GPIO_REG(pin), mask, val);
}

static int gpio_max20356_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_max20356_config *config = dev->config;
	gpio_port_value_t result = 0U;

	for (size_t pin = 0U; pin < MAX20356_GPIO_PINS; pin++) {
		uint8_t reg;
		int ret;

		ret = mfd_max20356_reg_read(config->mfd, MAX20356_GPIO_REG(pin), &reg);
		if (ret < 0) {
			return ret;
		}

		if (FIELD_GET(MAX20356_GPIO_PIN_MSK, reg) != 0U) {
			result |= BIT(pin);
		}
	}

	*value = result;

	return 0;
}

static int gpio_max20356_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	const struct gpio_max20356_config *config = dev->config;

	for (size_t pin = 0U; pin < MAX20356_GPIO_PINS; pin++) {
		uint8_t out;
		int ret;

		if ((mask & BIT(pin)) == 0U) {
			continue;
		}

		out = ((value & BIT(pin)) != 0U) ? MAX20356_GPIO_OUT_MSK : 0U;

		ret = mfd_max20356_reg_update(config->mfd, MAX20356_GPIO_REG(pin),
					      MAX20356_GPIO_OUT_MSK, out);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int gpio_max20356_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_max20356_port_set_masked_raw(dev, pins, pins);
}

static int gpio_max20356_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_max20356_port_set_masked_raw(dev, pins, 0U);
}

static int gpio_max20356_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_max20356_config *config = dev->config;

	for (size_t pin = 0U; pin < MAX20356_GPIO_PINS; pin++) {
		uint8_t reg;
		uint8_t out;
		int ret;

		if ((pins & BIT(pin)) == 0U) {
			continue;
		}

		ret = mfd_max20356_reg_read(config->mfd, MAX20356_GPIO_REG(pin), &reg);
		if (ret < 0) {
			return ret;
		}

		out = (FIELD_GET(MAX20356_GPIO_OUT_MSK, reg) != 0U) ? 0U : MAX20356_GPIO_OUT_MSK;

		ret = mfd_max20356_reg_update(config->mfd, MAX20356_GPIO_REG(pin),
					      MAX20356_GPIO_OUT_MSK, out);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(gpio, gpio_max20356_api) = {
	.pin_configure = gpio_max20356_pin_configure,
	.port_get_raw = gpio_max20356_port_get_raw,
	.port_set_masked_raw = gpio_max20356_port_set_masked_raw,
	.port_set_bits_raw = gpio_max20356_port_set_bits_raw,
	.port_clear_bits_raw = gpio_max20356_port_clear_bits_raw,
	.port_toggle_bits = gpio_max20356_port_toggle_bits,
};

static int gpio_max20356_init(const struct device *dev)
{
	const struct gpio_max20356_config *config = dev->config;

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	return 0;
}

#define GPIO_MAX20356_DEFINE(inst)                                                                 \
	static const struct gpio_max20356_config gpio_max20356_config_##inst = {                   \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
	};                                                                                         \
                                                                                                   \
	static struct gpio_max20356_data gpio_max20356_data_##inst;                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, gpio_max20356_init, NULL, &gpio_max20356_data_##inst,          \
			      &gpio_max20356_config_##inst, POST_KERNEL,                           \
			      CONFIG_GPIO_MAX20356_INIT_PRIORITY, &gpio_max20356_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MAX20356_DEFINE)
