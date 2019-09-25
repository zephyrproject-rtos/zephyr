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

static int pinmux_sam0_set(struct device *dev, u32_t pin, u32_t func)
{
	const struct pinmux_sam0_config *cfg = dev->config->config_info;
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

static int pinmux_sam0_get(struct device *dev, u32_t pin, u32_t *func)
{
	const struct pinmux_sam0_config *cfg = dev->config->config_info;
	bool odd_pin = pin & 1;
	int idx = pin / 2U;

	if (odd_pin) {
		*func = cfg->regs->PMUX[idx].bit.PMUXO;
	} else {
		*func = cfg->regs->PMUX[idx].bit.PMUXE;
	}

	return 0;
}

static int pinmux_sam0_pullup(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_sam0_input(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_sam0_init(struct device *dev)
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

#if DT_ATMEL_SAM0_PINMUX_PINMUX_A_BASE_ADDRESS
static const struct pinmux_sam0_config pinmux_sam0_config_0 = {
	.regs = (PortGroup *)DT_ATMEL_SAM0_PINMUX_PINMUX_A_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(pinmux_sam0_0, DT_ATMEL_SAM0_PINMUX_PINMUX_A_LABEL,
		    pinmux_sam0_init, NULL, &pinmux_sam0_config_0,
		    PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY,
		    &pinmux_sam0_api);
#endif

#if DT_ATMEL_SAM0_PINMUX_PINMUX_B_BASE_ADDRESS
static const struct pinmux_sam0_config pinmux_sam0_config_1 = {
	.regs = (PortGroup *)DT_ATMEL_SAM0_PINMUX_PINMUX_B_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(pinmux_sam0_1, DT_ATMEL_SAM0_PINMUX_PINMUX_B_LABEL,
		    pinmux_sam0_init, NULL, &pinmux_sam0_config_1,
		    PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY,
		    &pinmux_sam0_api);
#endif

#if DT_ATMEL_SAM0_PINMUX_PINMUX_C_BASE_ADDRESS
static const struct pinmux_sam0_config pinmux_sam0_config_2 = {
	.regs = (PortGroup *)DT_ATMEL_SAM0_PINMUX_PINMUX_C_BASE_ADDRESS,
};

DEVICE_AND_API_INIT(pinmux_sam0_2, DT_ATMEL_SAM0_PINMUX_PINMUX_C_LABEL,
		    pinmux_sam0_init, NULL, &pinmux_sam0_config_2,
		    PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY,
		    &pinmux_sam0_api);
#endif
