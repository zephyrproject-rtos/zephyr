/*
 * Copyright (c) 2017, NXP
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
#include <fsl_iocon.h>
#include <fsl_device_registers.h>

struct pinmux_mcux_lpc_config {
	clock_ip_name_t clock_ip_name;
	u32_t *base;
};

static int pinmux_mcux_lpc_set(struct device *dev, u32_t pin, u32_t func)
{
	const struct pinmux_mcux_lpc_config *config = dev->config->config_info;
	u32_t *base = config->base;

	base[pin] = func;

	return 0;
}

static int pinmux_mcux_lpc_get(struct device *dev, u32_t pin, u32_t *func)
{
	const struct pinmux_mcux_lpc_config *config = dev->config->config_info;
	u32_t *base = config->base;

	*func = base[pin];

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

#define PINMUX_LPC_INIT(n)						\
	static const struct pinmux_mcux_lpc_config			\
		pinmux_mcux_lpc_port##n##_cfg = {			\
			.base = (u32_t *)DT_INST_REG_ADDR(n),		\
			.clock_ip_name = kCLOCK_Iocon,			\
		};							\
									\
	DEVICE_AND_API_INIT(pinmux_port##n, DT_INST_LABEL(n),		\
			    &pinmux_mcux_lpc_init,			\
			    NULL, &pinmux_mcux_lpc_port##n##_cfg,	\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &pinmux_mcux_driver_api);

DT_INST_FOREACH(PINMUX_LPC_INIT)
