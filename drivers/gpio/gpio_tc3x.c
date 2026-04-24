/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tc3x_gpio

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/sys/util.h>
#include <errno.h>

#include "gpio_tc3x.h"

/**
 * @brief Common gpio flags to custom flags
 */
static int gpio_tc3x_flags_to_iocr(gpio_flags_t flags, uint32_t *iocr)
{
	bool is_input = flags & GPIO_INPUT;
	bool is_output = flags & GPIO_OUTPUT;

	/* Disconnect not supported */
	if (!is_input && !is_output) {
		return -ENOTSUP;
	}

	/* Open source not supported*/
	if (flags & GPIO_OPEN_SOURCE) {
		return -ENOTSUP;
	}

	/* Pull up & pull down not supported in output mode */
	if (is_output && (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if (is_input) {
		if (flags & (GPIO_PULL_UP)) {
			*iocr = TC3X_GPIO_MODE_INPUT_PULL_UP;
		} else if (flags & (GPIO_PULL_DOWN)) {
			*iocr = TC3X_GPIO_MODE_INPUT_PULL_DOWN;
		} else {
			*iocr = TC3X_GPIO_MODE_INPUT_TRISTATE;
		}
	}
	if (is_output) {
		if (flags & GPIO_OPEN_DRAIN) {
			*iocr = TC3X_GPIO_MODE_OUTPUT_OPEN_DRAIN;
		} else {
			*iocr = TC3X_GPIO_MODE_OUTPUT_PUSH_PULL;
		}
	}

	return 0;
}

#if defined(CONFIG_GPIO_GET_CONFIG)
static int gpio_tc3x_pincfg_to_flags(uint32_t iocr, uint32_t out, gpio_flags_t *out_flags)
{
	if (iocr & TC3X_IOCR_OUTPUT) {
		if (out) {
			*out_flags = GPIO_OUTPUT_HIGH;
		} else {
			*out_flags = GPIO_OUTPUT_LOW;
		}
		if (iocr & TC3X_IOCR_OPEN_DRAIN) {
			*out_flags |= GPIO_OPEN_DRAIN;
		}
	} else {
		*out_flags = GPIO_INPUT;
		if (iocr & TC3X_IOCR_PULL_DOWN) {
			*out_flags |= GPIO_PULL_DOWN;
		} else if (iocr & TC3X_IOCR_PULL_UP) {
			*out_flags |= GPIO_PULL_UP;
		}
	}

	return 0;
}
#endif

static int gpio_tc3x_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_tc3x_config *cfg = dev->config;

	*value = sys_read32(cfg->base + TC3X_IN_OFFSET);

	return 0;
}

static int gpio_tc3x_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct gpio_tc3x_config *cfg = dev->config;
	uint32_t clear, set;

	mask &= 0xFFFF;
	value &= 0xFFFF;

	set = (mask & value);
	clear = (mask & ~value);

	sys_write32((clear << 16) | set, cfg->base + TC3X_OMR_OFFSET);

	return 0;
}

static int gpio_tc3x_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tc3x_config *cfg = dev->config;

	sys_write32(0xFFFF & pins, cfg->base + TC3X_OMR_OFFSET);

	return 0;
}

static int gpio_tc3x_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tc3x_config *cfg = dev->config;

	sys_write32((0xFFFF & pins) << 16, cfg->base + TC3X_OMR_OFFSET);

	return 0;
}

static int gpio_tc3x_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tc3x_config *cfg = dev->config;
	uint32_t out;
	uint64_t swap;

	do {
		out = sys_read32(cfg->base + TC3X_OUT_OFFSET);
		swap = ((uint64_t)out << 32) | (out ^ pins);
		__asm("	cmpswap.w [%1]+0, %A0\n" : "+d"(swap) : "a"((void *)cfg->base));
	} while ((swap & 0xFFFFFFFF) != out);

	return 0;
}

/**
 * @brief Configure pin or port
 */
static int gpio_tc3x_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_tc3x_config *cfg = dev->config;
	int err;
	uint32_t iocr = 0;

	/* figure out if we can map the requested GPIO
	 * configuration
	 */
	err = gpio_tc3x_flags_to_iocr(flags, &iocr);
	if (err != 0) {
		return err;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_tc3x_port_set_bits_raw(dev, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_tc3x_port_clear_bits_raw(dev, BIT(pin));
		}
	}

	__asm("	imask %%e14, %0, %1, 5\n"
	      "	ldmst [%2]+0, %%e14\n"
	      :
	      : "d"(iocr), "d"((pin & 0x3) * 8 + 3),
		"a"(cfg->base + TC3X_IOCR_OFFSET + (pin >> 2)*4)
	      : "e14");

	return 0;
}

#if defined(CONFIG_GPIO_GET_CONFIG)
/**
 * @brief Get configuration of pin
 */
static int gpio_tc3x_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *flags)
{
	const struct gpio_tc3x_config *cfg = dev->config;

	gpio_tc3x_pincfg_to_flags(sys_read32(cfg->base + TC3X_IOCR_OFFSET),
				 sys_read32(cfg->base + TC3X_OUT_OFFSET), flags);

	return 0;
}
#endif

static int gpio_tc3x_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	return -ENOSYS;
}

static int gpio_tc3x_manage_callback(const struct device *dev, struct gpio_callback *callback,
				      bool set)
{
	return -ENOSYS;
}

static const struct gpio_driver_api gpio_tc3x_driver = {
	.pin_configure = gpio_tc3x_config,
#if defined(CONFIG_GPIO_GET_CONFIG)
	.pin_get_config = gpio_tc3x_get_config,
#endif /* CONFIG_GPIO_GET_CONFIG */
	.port_get_raw = gpio_tc3x_port_get_raw,
	.port_set_masked_raw = gpio_tc3x_port_set_masked_raw,
	.port_set_bits_raw = gpio_tc3x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_tc3x_port_clear_bits_raw,
	.port_toggle_bits = gpio_tc3x_port_toggle_bits,
	.pin_interrupt_configure = gpio_tc3x_pin_interrupt_configure,
	.manage_callback = gpio_tc3x_manage_callback,
};

/**
 * @brief Initialize GPIO port
 *
 * Perform basic initialization of a GPIO port. The code will
 * enable the clock for corresponding peripheral.
 *
 * @param dev GPIO device struct
 *
 * @return 0
 */
static int gpio_tc3x_init(const struct device *dev)
{
	return 0;
}

#define GPIO_TC3X_INIT(n)                                                                         \
	static const struct gpio_tc3x_config gpio_tc3x_config_##n = {                            \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
                                                                                                   \
	static struct gpio_tc3x_data gpio_tc3x_data_##n = {};                                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_tc3x_init, NULL, &gpio_tc3x_data_##n,                      \
			      &gpio_tc3x_config_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,     \
			      &gpio_tc3x_driver);

DT_INST_FOREACH_STATUS_OKAY(GPIO_TC3X_INIT)
