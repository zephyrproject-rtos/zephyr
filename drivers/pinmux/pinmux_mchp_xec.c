/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinmux.h>
#include <soc.h>
#include <zephyr/sys/printk.h>

#define DT_DRV_COMPAT microchip_xec_pinmux

static const uint32_t valid_ctrl_masks[NUM_MCHP_GPIO_PORTS] = {
	(MCHP_GPIO_PORT_A_BITMAP),
	(MCHP_GPIO_PORT_B_BITMAP),
	(MCHP_GPIO_PORT_C_BITMAP),
	(MCHP_GPIO_PORT_D_BITMAP),
	(MCHP_GPIO_PORT_E_BITMAP),
	(MCHP_GPIO_PORT_F_BITMAP)
};

struct pinmux_xec_config {
	uintptr_t pcr1_base;
	uint32_t port_num;
};

static int pinmux_xec_set(const struct device *dev, uint32_t pin,
			  uint32_t func)
{
	const struct pinmux_xec_config *config = dev->config;
	uintptr_t current_pcr1;
	uint32_t pcr1 = 0;
	uint32_t mask = 0;
	uint32_t temp = 0;

	/* Validate pin number in terms of current port */
	if ((valid_ctrl_masks[config->port_num] & BIT(pin)) == 0) {
		return -EINVAL;
	}

	mask |= MCHP_GPIO_CTRL_BUFT_MASK | MCHP_GPIO_CTRL_MUX_MASK |
		MCHP_GPIO_CTRL_INPAD_DIS_MASK;

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
	 * corresponds to the pin configured. Each pin control register
	 * on a 32-bit boundary.
	 */
	current_pcr1 = config->pcr1_base + pin * 4;
	temp = (sys_read32(current_pcr1) & ~mask) | pcr1;
	sys_write32(temp, current_pcr1);

	return 0;
}

static int pinmux_xec_get(const struct device *dev, uint32_t pin,
			  uint32_t *func)
{
	const struct pinmux_xec_config *config = dev->config;
	uintptr_t current_pcr1;

	/* Validate pin number in terms of current port */
	if ((valid_ctrl_masks[config->port_num] & BIT(pin)) == 0) {
		return -EINVAL;
	}

	current_pcr1 = config->pcr1_base + pin * 4;
	*func = sys_read32(current_pcr1) & (MCHP_GPIO_CTRL_BUFT_MASK
					    | MCHP_GPIO_CTRL_MUX_MASK
					    | MCHP_GPIO_CTRL_PUD_MASK);

	return 0;
}

static int pinmux_xec_pullup(const struct device *dev, uint32_t pin,
			     uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_xec_input(const struct device *dev, uint32_t pin,
			    uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_xec_init(const struct device *dev)
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

/* Get ph_reg address given a node-id */
#define PINMUX_ADDR(n) DT_REG_ADDR(DT_PHANDLE(n, ph_reg))
/* Get ph_reg address given instance */
#define PINMUX_INST_ADDR(n) DT_REG_ADDR(DT_PHANDLE(DT_NODELABEL(n), ph_reg))
/* Get port-num property */
#define PINMUX_PORT_NUM(n) DT_PROP(DT_NODELABEL(n), port_num)

/* id is a child node-id */
#define PINMUX_XEC_DEVICE(id)						\
	static const struct pinmux_xec_config pinmux_xec_port_cfg_##id = { \
		.pcr1_base = (uintptr_t)PINMUX_ADDR(id),		\
		.port_num = (uint32_t)DT_PROP(id, port_num),		\
	};								\
	DEVICE_DT_DEFINE(id, &pinmux_xec_init, NULL, NULL,		\
			 &pinmux_xec_port_cfg_##id,			\
			 PRE_KERNEL_1,					\
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,		\
			 &pinmux_xec_driver_api);

DT_FOREACH_CHILD_STATUS_OKAY(DT_NODELABEL(pinmux), PINMUX_XEC_DEVICE)
