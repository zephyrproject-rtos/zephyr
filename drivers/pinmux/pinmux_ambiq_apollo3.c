/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief PINMUX driver for the Ambiq Apollo3
 */

#include <errno.h>
#include <device.h>
#include <drivers/pinmux.h>
#include <soc.h>

#define DT_DRV_COMPAT ambiq_apollo3_pinmux


struct pinmux_ambiq_cfg {
	uintptr_t reg_padreg;
	uintptr_t reg_altpadcfg;
};

#define DEV_CFG(dev) ((const struct pinmux_ambiq_cfg *const)(dev)->config)

static int pinmux_ambiq_set(const struct device *dev, uint32_t pin,
			    uint32_t func)
{
	const struct pinmux_ambiq_config *config = DEV_CFG(dev);
	uint32_t reg;
	uint8_t val;
	int g, i;

	if (pin >= AMBIQ_PINMUX_PINS || func == NULL) {
		return -EINVAL;
	}
	g = pin / PIN_REG_OFFSET;
	i = pin % PIN_REG_OFFSET;
	reg = config->base + g;
	val = sys_read8(reg);
	if (func == IT8XXX2_PINMUX_IOF1) {
		sys_write8((val | IT8XXX2_PINMUX_IOF1 << i), reg);
	} else {
		sys_write8((val & ~(IT8XXX2_PINMUX_IOF1 << i)), reg);
	}

	return 0;
}

static int pinmux_ambiq_get(const struct device *dev, uint32_t pin,
			    uint32_t *func)
{
	const struct pinmux_ambiq_config *config = DEV_CFG(dev);
	uintptr_t reg;
	uint8_t val;

	if (pin >= AMBIQ_PINMUX_PINS || func == NULL) {
		return -EINVAL;
	}
	reg = config->reg_padreg + pin;
	val = sys_read8(reg);

	*func = val;

	return 0;
}

static int pinmux_ambiq_pullup(const struct device *dev, uint32_t pin,
			       uint8_t func)
{
	const struct pinmux_ambiq_config *config = DEV_CFG(dev);

	uintptr_t reg;
	uint8_t val;

	// TODO(michalc): IT8XXX2_PINMUX_PINS was here. Check where it is defined and add that
	// for Ambiq.
	if (pin >= AMBIQ_PINMUX_PINS) {
		return -EINVAL;
	}
	reg = config->reg_padreg + pin;
	val = sys_read8(reg);

	if (func == PINMUX_PULLUP_ENABLE) {
		val |= 0x1;
	} else if (func == PINMUX_PULLUP_DISABLE) {
		val &= ~(0x1);
	}

	sys_write8(val, reg);

	return 0;
}

static int pinmux_ambiq_input(const struct device *dev, uint32_t pin,
			      uint8_t func)
{
	const struct pinmux_ambiq_config *config = DEV_CFG(dev);

	uintptr_t reg;
	uint8_t val;

	// TODO(michalc): IT8XXX2_PINMUX_PINS was here. Check where it is defined and add that
	// for Ambiq.
	if (pin >= AMBIQ_PINMUX_PINS) {
		return -EINVAL;
	}
	reg = config->reg_padreg + pin;
	val = sys_read8(reg);

	if (func == PINMUX_INPUT_ENABLED) {
		val |= 0x1 << 1;
	} else if (func == PINMUX_OUTPUT_ENABLED) {
		val &= ~(0x1) << 1;
	}

	sys_write8(val, reg);

	return 0;
}

static int pinmux_ambiq_init(const struct device *dev)
{
	return 0;
}

static const struct pinmux_driver_api pinmux_ambiq_driver_api = {
	.set = pinmux_ambiq_set,
	.get = pinmux_ambiq_get,
	.pullup = pinmux_ambiq_pullup,
	.input = pinmux_ambiq_input,
};

static const struct pinmux_ambiq_config pinmux_ambiq_0_config = {
	// Read the properties of the DT_DRV_COMPAT compliant node.
	reg_padreg = DT_INST_REG_ADDR(0),
	reg_altpadcfg = DT_INST_REG_ADDR(1),
};

DEVICE_DT_INST_DEFINE(0, &pinmux_ambiq_init, device_pm_control_nop, NULL,
		      &pinmux_ambiq_0_config, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		      &pinmux_ambiq_driver_api);
