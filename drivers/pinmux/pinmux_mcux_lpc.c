/*
 * Copyright (c) 2017-2020, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <errno.h>
#include <device.h>
#include <drivers/pinmux.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <fsl_device_registers.h>

#define PORT0_IDX	0u
#define PORT1_IDX	1u

struct pinmux_mcux_lpc_config {
	clock_ip_name_t clock_ip_name;
#ifdef IOPCTL
	IOPCTL_Type *base;
#else
	IOCON_Type *base;
#endif
	uint32_t port_no;
};

static int pinmux_mcux_lpc_set(const struct device *dev, uint32_t pin,
			       uint32_t func)
{
	const struct pinmux_mcux_lpc_config *config = dev->config;
#ifdef IOPCTL
	IOPCTL_Type *base = config->base;
#else
	IOCON_Type *base = config->base;
#endif
	uint32_t port = config->port_no;

	base->PIO[port][pin] = func;

	return 0;
}

static int pinmux_mcux_lpc_get(const struct device *dev, uint32_t pin,
			       uint32_t *func)
{
	const struct pinmux_mcux_lpc_config *config = dev->config;
#ifdef IOPCTL
	IOPCTL_Type *base = config->base;
#else
	IOCON_Type *base = config->base;
#endif
	uint32_t port = config->port_no;

	*func = base->PIO[port][pin];

	return 0;
}

static int pinmux_mcux_lpc_pullup(const struct device *dev, uint32_t pin,
				  uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_mcux_lpc_input(const struct device *dev, uint32_t pin,
				 uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_mcux_lpc_init(const struct device *dev)
{
#ifndef IOPCTL
	const struct pinmux_mcux_lpc_config *config = dev->config;

	CLOCK_EnableClock(config->clock_ip_name);
#endif

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
#ifdef IOPCTL
	.base = IOPCTL,
#else
	.base = IOCON,
	.clock_ip_name = kCLOCK_Iocon,
#endif
	.port_no = PORT0_IDX,
};

DEVICE_DEFINE(pinmux_port0, CONFIG_PINMUX_MCUX_LPC_PORT0_NAME,
		    &pinmux_mcux_lpc_init, device_pm_control_nop,
		    NULL, &pinmux_mcux_lpc_port0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_mcux_driver_api);
#endif /* CONFIG_PINMUX_MCUX_LPC_PORT0 */

#ifdef CONFIG_PINMUX_MCUX_LPC_PORT1
static const struct pinmux_mcux_lpc_config pinmux_mcux_lpc_port1_config = {
#ifdef IOPCTL
	.base = IOPCTL,
#else
	.base = IOCON,
	.clock_ip_name = kCLOCK_Iocon,
#endif
	.port_no = PORT1_IDX,
};

DEVICE_DEFINE(pinmux_port1, CONFIG_PINMUX_MCUX_LPC_PORT1_NAME,
		    &pinmux_mcux_lpc_init, device_pm_control_nop,
		    NULL, &pinmux_mcux_lpc_port1_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_mcux_driver_api);
#endif /* CONFIG_PINMUX_MCUX_LPC_PORT1 */
