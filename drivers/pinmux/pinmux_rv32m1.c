/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_pinmux

#include <errno.h>
#include <device.h>
#include <drivers/pinmux.h>

#include <fsl_common.h>
#include <fsl_clock.h>

struct pinmux_rv32m1_config {
	clock_ip_name_t clock_ip_name;
	PORT_Type *base;
};

static int pinmux_rv32m1_set(const struct device *dev, uint32_t pin,
			     uint32_t func)
{
	const struct pinmux_rv32m1_config *config = dev->config;
	PORT_Type *base = config->base;

	base->PCR[pin] = (base->PCR[pin] & ~PORT_PCR_MUX_MASK) | func;

	return 0;
}

static int pinmux_rv32m1_get(const struct device *dev, uint32_t pin,
			     uint32_t *func)
{
	const struct pinmux_rv32m1_config *config = dev->config;
	PORT_Type *base = config->base;

	*func = base->PCR[pin] & ~PORT_PCR_MUX_MASK;

	return 0;
}

static int pinmux_rv32m1_pullup(const struct device *dev, uint32_t pin,
				uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_rv32m1_input(const struct device *dev, uint32_t pin,
			       uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_rv32m1_init(const struct device *dev)
{
	const struct pinmux_rv32m1_config *config = dev->config;

	CLOCK_EnableClock(config->clock_ip_name);

	return 0;
}

static const struct pinmux_driver_api pinmux_rv32m1_driver_api = {
	.set = pinmux_rv32m1_set,
	.get = pinmux_rv32m1_get,
	.pullup = pinmux_rv32m1_pullup,
	.input = pinmux_rv32m1_input,
};

#define PINMUX_RV32M1_INIT(n)						\
	static const struct pinmux_rv32m1_config pinmux_rv32m1_##n##_config = {\
		.base = (PORT_Type *)DT_INST_REG_ADDR(n),		\
		.clock_ip_name = INST_DT_CLOCK_IP_NAME(n),		\
	};								\
									\
	DEVICE_AND_API_INIT(pinmux_rv32m1_##n, DT_INST_LABEL(n),	\
			    &pinmux_rv32m1_init,			\
			    NULL, &pinmux_rv32m1_##n##_config,		\
			    PRE_KERNEL_1,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &pinmux_rv32m1_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PINMUX_RV32M1_INIT)
