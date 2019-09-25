/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/pinmux.h>
#include <fsl_common.h>
#include <fsl_clock.h>

struct pinmux_mcux_config {
	clock_ip_name_t clock_ip_name;
	PORT_Type *base;
};

static int pinmux_mcux_set(struct device *dev, u32_t pin, u32_t func)
{
	const struct pinmux_mcux_config *config = dev->config->config_info;
	PORT_Type *base = config->base;

	base->PCR[pin] = func;

	return 0;
}

static int pinmux_mcux_get(struct device *dev, u32_t pin, u32_t *func)
{
	const struct pinmux_mcux_config *config = dev->config->config_info;
	PORT_Type *base = config->base;

	*func = base->PCR[pin];

	return 0;
}

static int pinmux_mcux_pullup(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_mcux_input(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_mcux_init(struct device *dev)
{
	const struct pinmux_mcux_config *config = dev->config->config_info;

	CLOCK_EnableClock(config->clock_ip_name);

	return 0;
}

static const struct pinmux_driver_api pinmux_mcux_driver_api = {
	.set = pinmux_mcux_set,
	.get = pinmux_mcux_get,
	.pullup = pinmux_mcux_pullup,
	.input = pinmux_mcux_input,
};

#ifdef CONFIG_PINMUX_MCUX_PORTA
static const struct pinmux_mcux_config pinmux_mcux_porta_config = {
	.base = PORTA,
	.clock_ip_name = kCLOCK_PortA,
};

DEVICE_AND_API_INIT(pinmux_porta, CONFIG_PINMUX_MCUX_PORTA_NAME,
		    &pinmux_mcux_init,
		    NULL, &pinmux_mcux_porta_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_mcux_driver_api);
#endif

#ifdef CONFIG_PINMUX_MCUX_PORTB
static const struct pinmux_mcux_config pinmux_mcux_portb_config = {
	.base = PORTB,
	.clock_ip_name = kCLOCK_PortB,
};

DEVICE_AND_API_INIT(pinmux_portb, CONFIG_PINMUX_MCUX_PORTB_NAME,
		    &pinmux_mcux_init,
		    NULL, &pinmux_mcux_portb_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_mcux_driver_api);
#endif

#ifdef CONFIG_PINMUX_MCUX_PORTC
static const struct pinmux_mcux_config pinmux_mcux_portc_config = {
	.base = PORTC,
	.clock_ip_name = kCLOCK_PortC,
};

DEVICE_AND_API_INIT(pinmux_portc, CONFIG_PINMUX_MCUX_PORTC_NAME,
		    &pinmux_mcux_init,
		    NULL, &pinmux_mcux_portc_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_mcux_driver_api);
#endif

#ifdef CONFIG_PINMUX_MCUX_PORTD
static const struct pinmux_mcux_config pinmux_mcux_portd_config = {
	.base = PORTD,
	.clock_ip_name = kCLOCK_PortD,
};

DEVICE_AND_API_INIT(pinmux_portd, CONFIG_PINMUX_MCUX_PORTD_NAME,
		    &pinmux_mcux_init,
		    NULL, &pinmux_mcux_portd_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_mcux_driver_api);
#endif

#ifdef CONFIG_PINMUX_MCUX_PORTE
static const struct pinmux_mcux_config pinmux_mcux_porte_config = {
	.base = PORTE,
	.clock_ip_name = kCLOCK_PortE,
};

DEVICE_AND_API_INIT(pinmux_porte, CONFIG_PINMUX_MCUX_PORTE_NAME,
		    &pinmux_mcux_init,
		    NULL, &pinmux_mcux_porte_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_mcux_driver_api);
#endif
