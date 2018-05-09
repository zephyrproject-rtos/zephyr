/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <errno.h>
#include <device.h>
#include <pinmux.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <fsl_iocon.h>
#include <fsl_device_registers.h>

#define PORT0_IDX	0u
#define PORT1_IDX	1u

struct pinmux_mcux_lpc_config {
	clock_ip_name_t clock_ip_name;
	IOCON_Type *base;
	u32_t port_no;
};

static int pinmux_mcux_lpc_set(struct device *dev, u32_t pin, u32_t func)
{
	const struct pinmux_mcux_lpc_config *config = dev->config->config_info;
	IOCON_Type *base = config->base;
	u32_t port = config->port_no;

	base->PIO[port][pin] = func;

	return 0;
}

static int pinmux_mcux_lpc_get(struct device *dev, u32_t pin, u32_t *func)
{
	const struct pinmux_mcux_lpc_config *config = dev->config->config_info;
	IOCON_Type *base = config->base;
	u32_t port = config->port_no;

	*func = base->PIO[port][pin];

	return 0;
}

static int pinmux_mcux_lpc_pullup(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_mcux_lpc_input(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_mcux_lpc_init(struct device *dev)
{
	const struct pinmux_mcux_lpc_config *config = dev->config->config_info;

	CLOCK_EnableClock(config->clock_ip_name);

	return 0;
}

static const struct pinmux_driver_api pinmux_mcux_driver_api = {
	.set = pinmux_mcux_lpc_set,
	.get = pinmux_mcux_lpc_get,
	.pullup = pinmux_mcux_lpc_pullup,
	.input = pinmux_mcux_lpc_input,
};

#ifdef CONFIG_PINMUX_MCUX_LPC_PORT0
static const struct pinmux_mcux_lpc_config pinmux_mcux_lpc_port0_config = {
	.base = IOCON,
	.clock_ip_name = kCLOCK_Iocon,
	.port_no = PORT0_IDX,
};

DEVICE_AND_API_INIT(pinmux_port0, CONFIG_PINMUX_MCUX_LPC_PORT0_NAME,
		    &pinmux_mcux_lpc_init,
		    NULL, &pinmux_mcux_lpc_port0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_mcux_driver_api);
#endif /* CONFIG_PINMUX_MCUX_LPC_PORT0 */

#ifdef CONFIG_PINMUX_MCUX_LPC_PORT1
static const struct pinmux_mcux_lpc_config pinmux_mcux_lpc_port1_config = {
	.base = IOCON,
	.clock_ip_name = kCLOCK_Iocon,
	.port_no = PORT1_IDX,
};

DEVICE_AND_API_INIT(pinmux_port1, CONFIG_PINMUX_MCUX_LPC_PORT1_NAME,
		    &pinmux_mcux_lpc_init,
		    NULL, &pinmux_mcux_lpc_port1_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_mcux_driver_api);
#endif /* CONFIG_PINMUX_MCUX_LPC_PORT1 */
