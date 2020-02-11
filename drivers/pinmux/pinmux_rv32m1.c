/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/pinmux.h>

#include <fsl_common.h>
#include <fsl_clock.h>

struct pinmux_rv32m1_config {
	clock_ip_name_t clock_ip_name;
	PORT_Type *base;
};

static int pinmux_rv32m1_set(struct device *dev, u32_t pin, u32_t func)
{
	const struct pinmux_rv32m1_config *config = dev->config->config_info;
	PORT_Type *base = config->base;

	base->PCR[pin] = (base->PCR[pin] & ~PORT_PCR_MUX_MASK) | func;

	return 0;
}

static int pinmux_rv32m1_get(struct device *dev, u32_t pin, u32_t *func)
{
	const struct pinmux_rv32m1_config *config = dev->config->config_info;
	PORT_Type *base = config->base;

	*func = base->PCR[pin] & ~PORT_PCR_MUX_MASK;

	return 0;
}

static int pinmux_rv32m1_pullup(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_rv32m1_input(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_rv32m1_init(struct device *dev)
{
	const struct pinmux_rv32m1_config *config = dev->config->config_info;

	CLOCK_EnableClock(config->clock_ip_name);

	return 0;
}

static const struct pinmux_driver_api pinmux_rv32m1_driver_api = {
	.set = pinmux_rv32m1_set,
	.get = pinmux_rv32m1_get,
	.pullup = pinmux_rv32m1_pullup,
	.input = pinmux_rv32m1_input,
};

#ifdef CONFIG_PINMUX_RV32M1_PORTA
static const struct pinmux_rv32m1_config pinmux_rv32m1_porta_config = {
	.base = (PORT_Type *)DT_ALIAS_PINMUX_A_BASE_ADDRESS,
	.clock_ip_name = kCLOCK_PortA,
};

DEVICE_AND_API_INIT(pinmux_porta, CONFIG_PINMUX_RV32M1_PORTA_NAME,
		    &pinmux_rv32m1_init,
		    NULL, &pinmux_rv32m1_porta_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_rv32m1_driver_api);
#endif

#ifdef CONFIG_PINMUX_RV32M1_PORTB
static const struct pinmux_rv32m1_config pinmux_rv32m1_portb_config = {
	.base = (PORT_Type *)DT_ALIAS_PINMUX_B_BASE_ADDRESS,
	.clock_ip_name = kCLOCK_PortB,
};

DEVICE_AND_API_INIT(pinmux_portb, CONFIG_PINMUX_RV32M1_PORTB_NAME,
		    &pinmux_rv32m1_init,
		    NULL, &pinmux_rv32m1_portb_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_rv32m1_driver_api);
#endif

#ifdef CONFIG_PINMUX_RV32M1_PORTC
static const struct pinmux_rv32m1_config pinmux_rv32m1_portc_config = {
	.base = (PORT_Type *)DT_ALIAS_PINMUX_C_BASE_ADDRESS,
	.clock_ip_name = kCLOCK_PortC,
};

DEVICE_AND_API_INIT(pinmux_portc, CONFIG_PINMUX_RV32M1_PORTC_NAME,
		    &pinmux_rv32m1_init,
		    NULL, &pinmux_rv32m1_portc_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_rv32m1_driver_api);
#endif

#ifdef CONFIG_PINMUX_RV32M1_PORTD
static const struct pinmux_rv32m1_config pinmux_rv32m1_portd_config = {
	.base = (PORT_Type *)DT_ALIAS_PINMUX_D_BASE_ADDRESS,
	.clock_ip_name = kCLOCK_PortD,
};

DEVICE_AND_API_INIT(pinmux_portd, CONFIG_PINMUX_RV32M1_PORTD_NAME,
		    &pinmux_rv32m1_init,
		    NULL, &pinmux_rv32m1_portd_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_rv32m1_driver_api);
#endif

#ifdef CONFIG_PINMUX_RV32M1_PORTE
static const struct pinmux_rv32m1_config pinmux_rv32m1_porte_config = {
	.base = (PORT_Type *)DT_ALIAS_PINMUX_E_BASE_ADDRESS,
	.clock_ip_name = kCLOCK_PortE,
};

DEVICE_AND_API_INIT(pinmux_porte, CONFIG_PINMUX_RV32M1_PORTE_NAME,
		    &pinmux_rv32m1_init,
		    NULL, &pinmux_rv32m1_porte_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_rv32m1_driver_api);
#endif
