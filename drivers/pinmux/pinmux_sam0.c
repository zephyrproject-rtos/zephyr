/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pinmux.h>
#include <soc.h>

struct pinmux_sam0_config {
	PortGroup *regs;
};

static int pinmux_sam0_set(const struct device *dev, uint32_t pin,
			   uint32_t func)
{
	const struct pinmux_sam0_config *cfg = dev->config;
	bool odd_pin = pin & 1;
	int idx = pin / 2U;

	/* Each pinmux register holds the config for two pins.  The
	 * even numbered pin goes in the bits 0..3 and the odd
	 * numbered pin in bits 4..7.
	 */
	if (odd_pin) {
		cfg->regs->PMUX[idx].bit.PMUXO = func;
	} else {
		cfg->regs->PMUX[idx].bit.PMUXE = func;
	}
	cfg->regs->PINCFG[pin].bit.PMUXEN = 1;

	return 0;
}

static int pinmux_sam0_get(const struct device *dev, uint32_t pin,
			   uint32_t *func)
{
	const struct pinmux_sam0_config *cfg = dev->config;
	bool odd_pin = pin & 1;
	int idx = pin / 2U;

	if (odd_pin) {
		*func = cfg->regs->PMUX[idx].bit.PMUXO;
	} else {
		*func = cfg->regs->PMUX[idx].bit.PMUXE;
	}

	return 0;
}

static int pinmux_sam0_pullup(const struct device *dev, uint32_t pin,
			      uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_sam0_input(const struct device *dev, uint32_t pin,
			     uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_sam0_init(const struct device *dev)
{
	/* Nothing to do.  The GPIO clock is enabled at reset. */
	return 0;
}

const struct pinmux_driver_api pinmux_sam0_api = {
	.set = pinmux_sam0_set,
	.get = pinmux_sam0_get,
	.pullup = pinmux_sam0_pullup,
	.input = pinmux_sam0_input,
};

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_a), okay)
static const struct pinmux_sam0_config pinmux_sam0_config_0 = {
	.regs = (PortGroup *)DT_REG_ADDR(DT_NODELABEL(pinmux_a)),
};

DEVICE_AND_API_INIT(pinmux_sam0_0, DT_LABEL(DT_NODELABEL(pinmux_a)),
		    pinmux_sam0_init, NULL, &pinmux_sam0_config_0,
		    PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY,
		    &pinmux_sam0_api);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_b), okay)
static const struct pinmux_sam0_config pinmux_sam0_config_1 = {
	.regs = (PortGroup *)DT_REG_ADDR(DT_NODELABEL(pinmux_b)),
};

DEVICE_AND_API_INIT(pinmux_sam0_1, DT_LABEL(DT_NODELABEL(pinmux_b)),
		    pinmux_sam0_init, NULL, &pinmux_sam0_config_1,
		    PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY,
		    &pinmux_sam0_api);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_c), okay)
static const struct pinmux_sam0_config pinmux_sam0_config_2 = {
	.regs = (PortGroup *)DT_REG_ADDR(DT_NODELABEL(pinmux_c)),
};

DEVICE_AND_API_INIT(pinmux_sam0_2, DT_LABEL(DT_NODELABEL(pinmux_c)),
		    pinmux_sam0_init, NULL, &pinmux_sam0_config_2,
		    PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY,
		    &pinmux_sam0_api);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinmux_d), okay)
static const struct pinmux_sam0_config pinmux_sam0_config_3 = {
	.regs = (PortGroup *)DT_REG_ADDR(DT_NODELABEL(pinmux_d)),
};

DEVICE_AND_API_INIT(pinmux_sam0_3, DT_LABEL(DT_NODELABEL(pinmux_d)),
		    pinmux_sam0_init, NULL, &pinmux_sam0_config_3,
		    PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY,
		    &pinmux_sam0_api);
#endif
