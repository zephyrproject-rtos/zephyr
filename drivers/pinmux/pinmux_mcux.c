/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_pinmux

#include <errno.h>
#include <device.h>
#include <drivers/pinmux.h>
#include <fsl_common.h>
#include <fsl_clock.h>

struct pinmux_mcux_config {
	clock_ip_name_t clock_ip_name;
	PORT_Type *base;
};

static int pinmux_mcux_set(const struct device *dev, uint32_t pin,
			   uint32_t func)
{
	const struct pinmux_mcux_config *config = dev->config;
	PORT_Type *base = config->base;

	base->PCR[pin] = func;

	return 0;
}

static int pinmux_mcux_get(const struct device *dev, uint32_t pin,
			   uint32_t *func)
{
	const struct pinmux_mcux_config *config = dev->config;
	PORT_Type *base = config->base;

	*func = base->PCR[pin];

	return 0;
}

static int pinmux_mcux_pullup(const struct device *dev, uint32_t pin,
			      uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_mcux_input(const struct device *dev, uint32_t pin,
			     uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_mcux_init(const struct device *dev)
{
	const struct pinmux_mcux_config *config = dev->config;

	CLOCK_EnableClock(config->clock_ip_name);

	return 0;
}

static const struct pinmux_driver_api pinmux_mcux_driver_api = {
	.set = pinmux_mcux_set,
	.get = pinmux_mcux_get,
	.pullup = pinmux_mcux_pullup,
	.input = pinmux_mcux_input,
};

#if DT_NODE_HAS_STATUS(DT_INST(0, nxp_kinetis_ke1xf_sim), okay)
#define INST_DT_CLOCK_IP_NAME(n) \
	DT_REG_ADDR(DT_INST_PHANDLE(n, clocks)) + DT_INST_CLOCKS_CELL(n, name)
#else
#define INST_DT_CLOCK_IP_NAME(n) \
	CLK_GATE_DEFINE(DT_INST_CLOCKS_CELL(n, offset), \
			DT_INST_CLOCKS_CELL(n, bits))
#endif

#define PINMUX_MCUX_INIT(n)						\
	static const struct pinmux_mcux_config pinmux_mcux_##n##_config = {\
		.base = (PORT_Type *)DT_INST_REG_ADDR(n),		\
		.clock_ip_name = INST_DT_CLOCK_IP_NAME(n),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &pinmux_mcux_init,				\
			    device_pm_control_nop,			\
			    NULL, &pinmux_mcux_##n##_config,		\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &pinmux_mcux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PINMUX_MCUX_INIT)
