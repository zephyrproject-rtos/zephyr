/*
 * Copyright (c) 2017-2020, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_iocon_pio

#include <stdint.h>
#include <errno.h>
#include <device.h>
#include <drivers/pinmux.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <fsl_device_registers.h>

struct pinmux_mcux_lpc_config {
	clock_ip_name_t clock_ip_name;
	volatile uint32_t *base;
};

static int pinmux_mcux_lpc_set(const struct device *dev, uint32_t pin,
			       uint32_t func)
{
	const struct pinmux_mcux_lpc_config *config = dev->config;
	volatile uint32_t *base = config->base;

	base[pin] = func;

	return 0;
}

static int pinmux_mcux_lpc_get(const struct device *dev, uint32_t pin,
			       uint32_t *func)
{
	const struct pinmux_mcux_lpc_config *config = dev->config;
	volatile uint32_t *base = config->base;

	*func = base[pin];

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

#ifdef IOPCTL
#define LPC_CLOCK_IP_NAME kCLOCK_IpInvalid
#else
#define LPC_CLOCK_IP_NAME kCLOCK_Iocon
#endif

#define PINMUX_LPC_INIT(n)						\
	static const struct pinmux_mcux_lpc_config			\
		pinmux_mcux_lpc_port##n##_cfg = {			\
			.base = (uint32_t *)DT_INST_REG_ADDR(n),	\
			.clock_ip_name = LPC_CLOCK_IP_NAME,		\
		};							\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &pinmux_mcux_lpc_init,			\
			    device_pm_control_nop,			\
			    NULL, &pinmux_mcux_lpc_port##n##_cfg,	\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &pinmux_mcux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PINMUX_LPC_INIT)
