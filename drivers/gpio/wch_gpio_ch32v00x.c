/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/irq.h>

#include <ch32fun.h>

#define DT_DRV_COMPAT wch_gpio

struct gpio_ch32v00x_config {
	struct gpio_driver_config common;
	GPIO_TypeDef *regs;
	const struct device *clock_dev;
	uint8_t clock_id;
};

struct gpio_ch32v00x_data {
	struct gpio_driver_data common;
};

static int gpio_ch32v00x_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_ch32v00x_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;
	uint32_t cnf_mode;
	uint32_t bshr = 0;

	if ((flags & GPIO_OUTPUT) != 0) {
		cnf_mode = 0x01;
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			bshr = 1 << pin;
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			bshr = 1 << (16 + pin);
		}
	} else if ((flags & GPIO_INPUT) != 0) {
		if ((flags & GPIO_PULL_UP) != 0) {
			cnf_mode = GPIO_CFGLR_IN_PUPD;
			bshr = 1 << pin;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			cnf_mode = GPIO_CFGLR_IN_PUPD;
			bshr = 1 << (16 + pin);
		} else {
			cnf_mode = GPIO_CFGLR_IN_FLOAT;
		}
	} else {
		cnf_mode = 0x00;
	}

	if (pin < 8) {
		regs->CFGLR = (regs->CFGLR & ~(0x0F << (4 * pin))) | (cnf_mode << (4 * pin));
	} else {
		regs->CFGHR =
			(regs->CFGHR & ~(0x0F << ((pin - 8) * 4))) | (cnf_mode << ((pin - 8) * 4));
	}
	regs->BSHR = bshr;

	return 0;
}

static int gpio_ch32v00x_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_ch32v00x_config *config = dev->config;

	*value = config->regs->INDR;

	return 0;
}

static int gpio_ch32v00x_port_set_masked_raw(const struct device *dev, uint32_t mask,
					     uint32_t value)
{
	const struct gpio_ch32v00x_config *config = dev->config;

	config->regs->BSHR = ((~value & mask) << 16) | (value & mask);

	return 0;
}

static int gpio_ch32v00x_port_set_bits_raw(const struct device *dev, uint32_t pins)
{
	const struct gpio_ch32v00x_config *config = dev->config;

	config->regs->BSHR = pins;

	return 0;
}

static int gpio_ch32v00x_port_clear_bits_raw(const struct device *dev, uint32_t pins)
{
	const struct gpio_ch32v00x_config *config = dev->config;

	config->regs->BCR = pins;

	return 0;
}

static int gpio_ch32v00x_port_toggle_bits(const struct device *dev, uint32_t pins)
{
	const struct gpio_ch32v00x_config *config = dev->config;
	uint32_t changed = (config->regs->OUTDR ^ pins) & pins;

	config->regs->BSHR = (changed & pins) | (~changed & pins) << 16;

	return 0;
}

static DEVICE_API(gpio, gpio_ch32v00x_driver_api) = {
	.pin_configure = gpio_ch32v00x_configure,
	.port_get_raw = gpio_ch32v00x_port_get_raw,
	.port_set_masked_raw = gpio_ch32v00x_port_set_masked_raw,
	.port_set_bits_raw = gpio_ch32v00x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ch32v00x_port_clear_bits_raw,
	.port_toggle_bits = gpio_ch32v00x_port_toggle_bits,
};

static int gpio_ch32v00x_init(const struct device *dev)
{
	const struct gpio_ch32v00x_config *config = dev->config;

	clock_control_on(config->clock_dev, (clock_control_subsys_t *)(uintptr_t)config->clock_id);

	return 0;
}

#define GPIO_CH32V00X_INIT(idx)                                                                    \
	static const struct gpio_ch32v00x_config gpio_ch32v00x_##idx##_config = {                  \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),             \
			},                                                                         \
		.regs = (GPIO_TypeDef *)DT_INST_REG_ADDR(idx),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_id = DT_INST_CLOCKS_CELL(idx, id),                                          \
	};                                                                                         \
                                                                                                   \
	static struct gpio_ch32v00x_data gpio_ch32v00x_##idx##_data;                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, gpio_ch32v00x_init, NULL, &gpio_ch32v00x_##idx##_data,          \
			      &gpio_ch32v00x_##idx##_config, PRE_KERNEL_1,                         \
			      CONFIG_GPIO_INIT_PRIORITY, &gpio_ch32v00x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CH32V00X_INIT)
