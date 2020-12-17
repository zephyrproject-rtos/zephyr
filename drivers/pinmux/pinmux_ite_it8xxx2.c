/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief PINMUX driver for the IT8xxx2
 */

#include <errno.h>
#include <device.h>
#include <drivers/pinmux.h>
#include <soc.h>

#define DT_DRV_COMPAT		ite_it8xxx2_pinmux
#define PIN_REG_OFFSET		8

struct pinmux_it8xxx2_config {
	uintptr_t base;
};

#define DEV_CFG(dev)					\
	((const struct pinmux_it8xxx2_config * const)	\
	 (dev)->config)

static int pinmux_it8xxx2_set(const struct device *dev,
		uint32_t pin, uint32_t func)
{
	const struct pinmux_it8xxx2_config *config = DEV_CFG(dev);
	uint32_t reg;
	uint8_t val;
	int g, i;

	if (func > IT8XXX2_PINMUX_IOF1 || pin >= IT8XXX2_PINMUX_PINS) {
		return -EINVAL;
	}
	g =  pin / PIN_REG_OFFSET;
	i =  pin % PIN_REG_OFFSET;
	reg = config->base + g;
	val = sys_read8(reg);
	if (func == IT8XXX2_PINMUX_IOF1) {
		sys_write8((val | IT8XXX2_PINMUX_IOF1 << i), reg);
	} else {
		sys_write8((val & ~(IT8XXX2_PINMUX_IOF1 << i)), reg);
	}

	return 0;
}

static int pinmux_it8xxx2_get(const struct device *dev,
		uint32_t pin, uint32_t *func)
{
	const struct pinmux_it8xxx2_config *config = DEV_CFG(dev);
	uint32_t reg;
	uint8_t val;
	int g, i;

	if (pin >= IT8XXX2_PINMUX_PINS || func == NULL) {
		return -EINVAL;
	}
	g =  pin / PIN_REG_OFFSET;
	i =  pin % PIN_REG_OFFSET;
	reg = config->base + g;
	val = sys_read8(reg);
	*func = (val & (IT8XXX2_PINMUX_IOF1 << pin)) ?
		IT8XXX2_PINMUX_IOF1 : IT8XXX2_PINMUX_IOF0;

	return 0;
}

static int pinmux_it8xxx2_pullup(const struct device *dev,
		uint32_t pin, uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_it8xxx2_input(const struct device *dev,
		uint32_t pin, uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_it8xxx2_init(const struct device *dev)
{
	return 0;
}

static const struct pinmux_driver_api pinmux_it8xxx2_driver_api = {
	.set = pinmux_it8xxx2_set,
	.get = pinmux_it8xxx2_get,
	.pullup = pinmux_it8xxx2_pullup,
	.input = pinmux_it8xxx2_input,
};

static const struct pinmux_it8xxx2_config pinmux_it8xxx2_0_config = {
	.base = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, &pinmux_it8xxx2_init, device_pm_control_nop,
		    NULL, &pinmux_it8xxx2_0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_it8xxx2_driver_api);
