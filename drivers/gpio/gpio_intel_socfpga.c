/*
 * Copyright (c) 2022, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_socfpga_gpio

/**
 * @file
 * @brief Intel GPIO SoCFPGA Controller Driver
 *
 * The GPIO controller on Intel SoCFPGA serves
 * both GPIO modules. This driver provides
 * the GPIO function.
 *
 */

#include <errno.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include "gpio_intel_socfpga.h"

#define DEV_CFG(_dev) ((struct gpio_intel_socfpga_config *const)(_dev)->config)

struct gpio_intel_socfpga_config {
	DEVICE_MMIO_ROM;
	const int port_pin_mask;
	const int gpio_port;
	uint32_t ngpios;
	struct reset_dt_spec reset;
};

struct gpio_intel_socfpga_data {
	DEVICE_MMIO_RAM;
};

static bool pmux_gpio_pin_is_valid(int port, uint32_t pin_mask,
				   const int port_pin_mask, uint32_t ngpios)
{
	uint32_t pin;
	uint32_t hps_io_pin;
	uint32_t addroffst;
	uint32_t pmux_value;

	/* Pin range more than 32 will return 0x0 pin mask */
	if (pin_mask == 0) {
		return false;
	}

	while (pin_mask != 0) {
		pin = find_lsb_set(pin_mask) - 1;

		/* Check whether input pin is in range */
		if ((BIT(pin) & port_pin_mask) == 0) {
			return false;
		}

		/* Check for GPIO pinmux configuration value */
		hps_io_pin = (port * ngpios) + pin;
		addroffst = hps_io_pin * PMUX_SEL_OFFSET;

		if (hps_io_pin >= PMUX_40_SEL)
			addroffst += PMUX_40_SEL_GAP;

		pmux_value = sys_read32((SOCFPGA_PINMUX_PIN0SEL_REG_BASE + addroffst));

		if (pmux_value != PMUX_GPIO_VAL) {
			return false;
		}

		pin_mask = pin_mask ^ BIT(pin);
	}

	return true;
}

static int gpio_socfpga_configure(const struct device *dev,
			      gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_intel_socfpga_config *const cfg = DEV_CFG(dev);
	const int port_pin_mask = cfg->port_pin_mask;
	const int gpio_port = cfg->gpio_port;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t ngpios = cfg->ngpios;
	uint32_t addr;

	addr = reg_base + GPIO_SWPORTA_DDR_OFFSET;

	if (!pmux_gpio_pin_is_valid(gpio_port, BIT(pin), port_pin_mask, ngpios)) {
		return -EINVAL;
	}

	if (flags & GPIO_INPUT) {
		sys_clear_bits(addr, BIT(pin));
	} else if (flags & GPIO_OUTPUT) {
		sys_set_bits(addr, BIT(pin));
	}

	return 0;
}

static int gpio_socfpga_port_get_raw(const struct device *dev, uint32_t *value)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t addr;

	addr = reg_base + GPIO_EXT_PORTA_OFFSET;

	*value = sys_read32((addr));

	return 0;
}

static int gpio_socfpga_port_set_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	struct gpio_intel_socfpga_config *const cfg = DEV_CFG(dev);
	const int port_pin_mask = cfg->port_pin_mask;
	const int gpio_port = cfg->gpio_port;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t ngpios = cfg->ngpios;

	if (!pmux_gpio_pin_is_valid(gpio_port, mask, port_pin_mask, ngpios)) {
		return -EINVAL;
	}

	sys_set_bits(reg_base, mask);

	return 0;
}

static int gpio_socfpga_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	struct gpio_intel_socfpga_config *const cfg = DEV_CFG(dev);
	const int port_pin_mask = cfg->port_pin_mask;
	const int gpio_port = cfg->gpio_port;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t ngpios = cfg->ngpios;

	if (!pmux_gpio_pin_is_valid(gpio_port, mask, port_pin_mask, ngpios)) {
		return -EINVAL;
	}

	sys_clear_bits(reg_base, mask);

	return 0;
}

static int gpio_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	struct gpio_intel_socfpga_config *const cfg = DEV_CFG(dev);

	if (!device_is_ready(cfg->reset.dev)) {
		return -ENODEV;
	}

	reset_line_assert(cfg->reset.dev, cfg->reset.id);
	reset_line_deassert(cfg->reset.dev, cfg->reset.id);

	return 0;
}

static const struct gpio_driver_api gpio_socfpga_driver_api = {
	.pin_configure = gpio_socfpga_configure,
	.port_get_raw = gpio_socfpga_port_get_raw,
	.port_set_masked_raw = NULL,
	.port_set_bits_raw = gpio_socfpga_port_set_bits_raw,
	.port_clear_bits_raw = gpio_socfpga_port_clear_bits_raw,
	.port_toggle_bits = NULL,
	.pin_interrupt_configure = NULL,
	.manage_callback = NULL
};

#define CREATE_GPIO_DEVICE(_inst)						\
	static struct gpio_intel_socfpga_data gpio_data_##_inst;		\
	static struct gpio_intel_socfpga_config gpio_config_##_inst = {		\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(_inst)),			\
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(_inst),	\
		.gpio_port = _inst,						\
		.ngpios = DT_INST_PROP(_inst, ngpios),				\
		.reset = RESET_DT_SPEC_INST_GET(_inst)			\
	};									\
	DEVICE_DT_INST_DEFINE(_inst,						\
			gpio_init,						\
			NULL,							\
			&gpio_data_##_inst,					\
			&gpio_config_##_inst,					\
			PRE_KERNEL_1,						\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
			&gpio_socfpga_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CREATE_GPIO_DEVICE)
