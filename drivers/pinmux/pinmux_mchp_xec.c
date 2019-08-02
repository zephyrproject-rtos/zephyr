/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/pinmux.h>
#include <soc.h>

static const u32_t valid_ctrl_masks[NUM_MCHP_GPIO_PORTS] = {
	(MCHP_GPIO_PORT_A_BITMAP),
	(MCHP_GPIO_PORT_B_BITMAP),
	(MCHP_GPIO_PORT_C_BITMAP),
	(MCHP_GPIO_PORT_D_BITMAP),
	(MCHP_GPIO_PORT_E_BITMAP),
	(MCHP_GPIO_PORT_F_BITMAP)
};

struct pinmux_xec_config {
	__IO u32_t *pcr1_base;
	u32_t port_num;
};

static int pinmux_xec_set(struct device *dev, u32_t pin, u32_t func)
{
	const struct pinmux_xec_config *config = dev->config->config_info;
	__IO u32_t *current_pcr1;
	u32_t pcr1 = 0;
	u32_t mask = 0;

	/* Validate pin number in terms of current port */
	if ((valid_ctrl_masks[config->port_num] & BIT(pin)) == 0) {
		return -EINVAL;
	}

	mask |= MCHP_GPIO_CTRL_BUFT_MASK | MCHP_GPIO_CTRL_MUX_MASK;

	/* Check for open drain/push_pull setting */
	if (func & MCHP_GPIO_CTRL_BUFT_OPENDRAIN) {
		pcr1 |= MCHP_GPIO_CTRL_BUFT_OPENDRAIN;
	} else {
		pcr1 |= MCHP_GPIO_CTRL_BUFT_PUSHPULL;
	}

	/* Parse mux mode */
	pcr1 |= func & MCHP_GPIO_CTRL_MUX_MASK;

	/* Figure out the pullup/pulldown configuration */
	mask |= MCHP_GPIO_CTRL_PUD_MASK;

	if (func & MCHP_GPIO_CTRL_PUD_PU) {
		/* Enable the pull and select the pullup resistor. */
		pcr1 |= MCHP_GPIO_CTRL_PUD_PU;
	} else if (func & MCHP_GPIO_CTRL_PUD_PD) {
		/* Enable the pull and select the pulldown resistor */
		pcr1 |= MCHP_GPIO_CTRL_PUD_PD;
	} else {
		/* None : Pin tristates when no active driver is present
		 * on the pin. This is the POR setting
		 */
		pcr1 |= MCHP_GPIO_CTRL_PUD_NONE;
	}

	/* Make sure gpio isrs are disabled */
	pcr1 |= MCHP_GPIO_CTRL_IDET_DISABLE;
	mask |= MCHP_GPIO_CTRL_IDET_MASK;

	/* Now write contents of pcr1 variable to the PCR1 register that
	 * corresponds to the pin configured
	 */
	current_pcr1 = config->pcr1_base + pin;
	*current_pcr1 = (*current_pcr1 & ~mask) | pcr1;

	return 0;
}

static int pinmux_xec_get(struct device *dev, u32_t pin, u32_t *func)
{
	const struct pinmux_xec_config *config = dev->config->config_info;
	__IO u32_t *current_pcr1;

	/* Validate pin number in terms of current port */
	if ((valid_ctrl_masks[config->port_num] & BIT(pin)) == 0) {
		return -EINVAL;
	}

	current_pcr1 = config->pcr1_base + pin;
	*func = *current_pcr1 & (MCHP_GPIO_CTRL_BUFT_MASK
					| MCHP_GPIO_CTRL_MUX_MASK
					| MCHP_GPIO_CTRL_PUD_MASK);

	return 0;
}

static int pinmux_xec_pullup(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_xec_input(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_xec_init(struct device *dev)
{
	/* Nothing to do. The PCR clock is enabled at reset. */
	return 0;
}

static const struct pinmux_driver_api pinmux_xec_driver_api = {
	.set = pinmux_xec_set,
	.get = pinmux_xec_get,
	.pullup = pinmux_xec_pullup,
	.input = pinmux_xec_input,
};

#ifdef CONFIG_PINMUX_XEC_GPIO000_036
static const struct pinmux_xec_config pinmux_xec_port000_036_config = {
	.pcr1_base = (u32_t *) DT_PINMUX_XEC_GPIO000_036_BASE_ADDR,
	.port_num = MCHP_GPIO_000_036,
};

DEVICE_AND_API_INIT(pinmux_xec_port000_036, CONFIG_PINMUX_XEC_GPIO000_036_NAME,
		    &pinmux_xec_init,
		    NULL, &pinmux_xec_port000_036_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_xec_driver_api);
#endif /* CONFIG_PINMUX_XEC_GPIO000_036 */

#ifdef CONFIG_PINMUX_XEC_GPIO040_076
static const struct pinmux_xec_config pinmux_xec_port040_076_config = {
	.pcr1_base = (u32_t *) DT_PINMUX_XEC_GPIO040_076_BASE_ADDR,
	.port_num = MCHP_GPIO_040_076,
};

DEVICE_AND_API_INIT(pinmux_xec_port040_076, CONFIG_PINMUX_XEC_GPIO040_076_NAME,
		    &pinmux_xec_init,
		    NULL, &pinmux_xec_port040_076_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_xec_driver_api);
#endif /* CONFIG_PINMUX_XEC_GPIO040_076 */

#ifdef CONFIG_PINMUX_XEC_GPIO100_136
static const struct pinmux_xec_config pinmux_xec_port100_136_config = {
	.pcr1_base = (u32_t *)  DT_PINMUX_XEC_GPIO100_136_BASE_ADDR,
	.port_num = MCHP_GPIO_100_136,
};

DEVICE_AND_API_INIT(pinmux_xec_port100_136, CONFIG_PINMUX_XEC_GPIO100_136_NAME,
		    &pinmux_xec_init,
		    NULL, &pinmux_xec_port100_136_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_xec_driver_api);
#endif /* CONFIG_PINMUX_XEC_GPIO100_136 */

#ifdef CONFIG_PINMUX_XEC_GPIO140_176
static const struct pinmux_xec_config pinmux_xec_port140_176_config = {
	.pcr1_base = (u32_t *)  DT_PINMUX_XEC_GPIO140_176_BASE_ADDR,
	.port_num = MCHP_GPIO_140_176,
};

DEVICE_AND_API_INIT(pinmux_xec_port140_176, CONFIG_PINMUX_XEC_GPIO140_176_NAME,
		    &pinmux_xec_init,
		    NULL, &pinmux_xec_port140_176_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_xec_driver_api);
#endif /* CONFIG_PINMUX_XEC_GPIO140_176 */

#ifdef CONFIG_PINMUX_XEC_GPIO200_236
static const struct pinmux_xec_config pinmux_xec_port200_236_config = {
	.pcr1_base = (u32_t *) DT_PINMUX_XEC_GPIO200_236_BASE_ADDR,
	.port_num = MCHP_GPIO_200_236,
};

DEVICE_AND_API_INIT(pinmux_xec_port200_236, CONFIG_PINMUX_XEC_GPIO200_236_NAME,
		    &pinmux_xec_init,
		    NULL, &pinmux_xec_port200_236_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_xec_driver_api);
#endif /* CONFIG_PINMUX_XEC_GPIO200_236 */

#ifdef CONFIG_PINMUX_XEC_GPIO240_276
static const struct pinmux_xec_config pinmux_xec_port240_276_config = {
	.pcr1_base = (u32_t *)  DT_PINMUX_XEC_GPIO240_276_BASE_ADDR,
	.port_num = MCHP_GPIO_240_276,
};

DEVICE_AND_API_INIT(pinmux_xec_port240_276, CONFIG_PINMUX_XEC_GPIO240_276_NAME,
		    &pinmux_xec_init,
		    NULL, &pinmux_xec_port240_276_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_xec_driver_api);
#endif /* CONFIG_PINMUX_XEC_GPIO240_276 */
