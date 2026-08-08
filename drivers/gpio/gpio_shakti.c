/*
 * Copyright (c) 2026 RISE Lab, IIT Madras
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT shakti_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* Register offsets from the GPIO base */
#define GPIO_DIR	0x00
#define GPIO_DATA	0x08
#define GPIO_SET	0x10
#define GPIO_CLEAR	0x18
#define GPIO_TOGGLE	0x20

struct gpio_shakti_config {
	struct gpio_driver_config common;
	uintptr_t base;
};

struct gpio_shakti_data {
	struct gpio_driver_data common;
};

static int gpio_shakti_pin_configure(const struct device *dev, gpio_pin_t pin,
				     gpio_flags_t flags)
{
	const struct gpio_shakti_config *cfg = dev->config;
	uint32_t dir = sys_read32(cfg->base + GPIO_DIR);

	if (flags & GPIO_OUTPUT) {
		dir |= BIT(pin);
		sys_write32(dir, cfg->base + GPIO_DIR);
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			sys_write32(BIT(pin), cfg->base + GPIO_SET);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			sys_write32(BIT(pin), cfg->base + GPIO_CLEAR);
		}
	} else if (flags & GPIO_INPUT) {
		dir &= ~BIT(pin);
		sys_write32(dir, cfg->base + GPIO_DIR);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_shakti_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_shakti_config *cfg = dev->config;

	*value = sys_read32(cfg->base + GPIO_DATA);
	return 0;
}

static int gpio_shakti_port_set_masked_raw(const struct device *dev,
					   gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	const struct gpio_shakti_config *cfg = dev->config;
	uint32_t data = sys_read32(cfg->base + GPIO_DATA);

	data = (data & ~mask) | (value & mask);
	sys_write32(data, cfg->base + GPIO_DATA);
	return 0;
}

static int gpio_shakti_port_set_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_shakti_config *cfg = dev->config;

	sys_write32(mask, cfg->base + GPIO_SET);
	return 0;
}

static int gpio_shakti_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_shakti_config *cfg = dev->config;

	sys_write32(mask, cfg->base + GPIO_CLEAR);
	return 0;
}

static int gpio_shakti_port_toggle_bits(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_shakti_config *cfg = dev->config;

	sys_write32(mask, cfg->base + GPIO_TOGGLE);
	return 0;
}

static DEVICE_API(gpio, gpio_shakti_driver_api) = {
	.pin_configure = gpio_shakti_pin_configure,
	.port_get_raw = gpio_shakti_port_get_raw,
	.port_set_masked_raw = gpio_shakti_port_set_masked_raw,
	.port_set_bits_raw = gpio_shakti_port_set_bits_raw,
	.port_clear_bits_raw = gpio_shakti_port_clear_bits_raw,
	.port_toggle_bits = gpio_shakti_port_toggle_bits,
};

#define GPIO_SHAKTI_INIT(n)                                         \
	static const struct gpio_shakti_config gpio_shakti_cfg_##n = {  \
		.common = {                                                 \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),    \
		},                                                          \
		.base = DT_INST_REG_ADDR(n),                                \
	};                                                              \
	static struct gpio_shakti_data gpio_shakti_data_##n;            \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL,                            \
			      &gpio_shakti_data_##n, &gpio_shakti_cfg_##n,      \
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,          \
			      &gpio_shakti_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_SHAKTI_INIT)
