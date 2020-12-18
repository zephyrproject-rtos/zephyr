/*
 * Copyright (c) 2019 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/pinmux.h>
#include <soc.h>

#define creg_gpio_mux_reg	(*(volatile uint32_t *)CREG_GPIO_MUX_BASE_ADDR)

void _arc_sync(void)
{
	__asm__ volatile("sync");
}

static int pinmux_hsdk_set(const struct device *dev, uint32_t pin,
			   uint32_t func)
{

	if (func >= HSDK_PINMUX_FUNS || pin >= HSDK_PINMUX_SELS)
		return -EINVAL;

	creg_gpio_mux_reg &= ~(0x07U << (pin * 3));
	creg_gpio_mux_reg |= (func << (pin * 3));

	_arc_sync();

	return 0;
}

static int pinmux_hsdk_get(const struct device *dev, uint32_t pin,
			   uint32_t *func)
{

	if (pin >= HSDK_PINMUX_SELS || func == NULL)
		return -EINVAL;

	*func = (creg_gpio_mux_reg >> (pin * 3)) & 0x07U;

	return 0;
}

static int pinmux_hsdk_pullup(const struct device *dev, uint32_t pin,
			      uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_hsdk_input(const struct device *dev, uint32_t pin,
			     uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_hsdk_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static const struct pinmux_driver_api pinmux_hsdk_driver_api = {
	.set = pinmux_hsdk_set,
	.get = pinmux_hsdk_get,
	.pullup = pinmux_hsdk_pullup,
	.input = pinmux_hsdk_input,
};

DEVICE_DEFINE(pinmux_hsdk, CONFIG_PINMUX_NAME,
		&pinmux_hsdk_init, device_pm_control_nop, NULL, NULL,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&pinmux_hsdk_driver_api);
