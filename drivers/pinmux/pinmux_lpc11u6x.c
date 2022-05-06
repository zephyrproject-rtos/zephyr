/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc11u6x_pinmux

/**
 * @file
 * @brief pinmux driver for NXP LPC11U6X SoCs
 *
 * This driver allows to configure the IOCON (I/O control) registers found
 * on the LPC11U6x MCUs.
 *
 * The IOCON registers are divided into three ports. The number of pins
 * available on each port depends on the package type (48, 64 or 100 pins).
 * Each port is handled as a distinct device and is defined by a dedicated
 * device tree node. This node provides the port's base address and number
 * of pins information.
 */

#include <zephyr/drivers/pinmux.h>

struct pinmux_lpc11u6x_config {
	uint8_t port;
	volatile uint32_t *base;
	uint8_t npins;
};

static int pinmux_lpc11u6x_set(const struct device *dev, uint32_t pin,
			       uint32_t func)
{
	const struct pinmux_lpc11u6x_config *config = dev->config;
	volatile uint32_t *base;

	if (pin >= config->npins) {
		return -EINVAL;
	}

	/* Handle 4 bytes hole between PIO2_1 and PIO2_2. */
	if (config->port == 2 && pin > 1) {
		base = config->base + 1;
	} else {
		base = config->base;
	}

	base[pin] = func;

	return 0;
}

static int
pinmux_lpc11u6x_get(const struct device *dev, uint32_t pin, uint32_t *func)
{
	const struct pinmux_lpc11u6x_config *config = dev->config;
	volatile uint32_t *base;

	if (pin >= config->npins) {
		return -EINVAL;
	}

	/* Handle 4 bytes hole between PIO2_1 and PIO2_2. */
	if (config->port == 2 && pin > 1) {
		base = config->base + 1;
	} else {
		base = config->base;
	}

	*func = base[pin];

	return 0;
}

static int
pinmux_lpc11u6x_pullup(const struct device *dev, uint32_t pin, uint8_t func)
{
	return -ENOTSUP;
}

static int
pinmux_lpc11u6x_input(const struct device *dev, uint32_t pin, uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_lpc11u6x_init(const struct device *dev)
{
	return 0;
}

static const struct pinmux_driver_api pinmux_lpc11u6x_driver_api = {
	.set = pinmux_lpc11u6x_set,
	.get = pinmux_lpc11u6x_get,
	.pullup = pinmux_lpc11u6x_pullup,
	.input = pinmux_lpc11u6x_input,
};

#define PINMUX_LPC11U6X_INIT(id)				\
static const struct pinmux_lpc11u6x_config			\
			pinmux_lpc11u6x_config_##id = {		\
	.port = id,						\
	.base = (volatile uint32_t *) DT_INST_REG_ADDR(id),	\
	.npins = DT_INST_REG_SIZE(id) / 4,			\
};								\
								\
DEVICE_DT_INST_DEFINE(id, &pinmux_lpc11u6x_init,		\
		    NULL, NULL, &pinmux_lpc11u6x_config_##id,	\
		    PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY,	\
		    &pinmux_lpc11u6x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PINMUX_LPC11U6X_INIT)
