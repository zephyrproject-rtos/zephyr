/*
 * Copyright (c) 2016 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <board.h>

#include <device.h>
#include <errno.h>
#include <init.h>
#include <pinmux.h>
#include <soc.h>
#include <gpio/gpio_cmsdk_ahb.h>
#include <misc/util.h>

/* Number of pins for each port */
#define PINS_PER_PORT    16

#define CMSDK_AHB_GPIO0_DEV \
	((volatile struct gpio_cmsdk_ahb *)CMSDK_AHB_GPIO0)
#define CMSDK_AHB_GPIO1_DEV \
	((volatile struct gpio_cmsdk_ahb *)CMSDK_AHB_GPIO1)

static volatile struct gpio_cmsdk_ahb *_get_port(uint32_t pin)
{
	uint32_t port_num = pin / PINS_PER_PORT;

	/* Port 2 and 3 are reserved therefore not handled in this driver */
	switch (port_num) {
	case 0:
		return CMSDK_AHB_GPIO0_DEV;
	case 1:
		return CMSDK_AHB_GPIO1_DEV;
	default:
		/* return null if pin is outside range */
		return NULL;
	}
}

static int pinmux_set(struct device *dev, uint32_t pin, uint32_t func)
{
	volatile struct gpio_cmsdk_ahb *port = _get_port(pin);
	uint32_t tmp;
	uint32_t key;

	ARG_UNUSED(dev);

	if (!port) {
		return -EINVAL;
	}

	if (func) {
		/*
		 * The irq_lock() here is required to prevent concurrent callers
		 * to corrupt the pin functions.
		 */
		key = irq_lock();

		tmp = port->altfuncset;
		tmp |= (1 << (pin % PINS_PER_PORT));
		port->altfuncset = tmp;

		irq_unlock(key);
	} else {
		/*
		 * The irq_lock() here is required to prevent concurrent callers
		 * to corrupt the pin functions.
		 */
		key = irq_lock();

		tmp = port->altfuncclr;
		tmp |= (1 << (pin % PINS_PER_PORT));
		port->altfuncclr = tmp;

		irq_unlock(key);
	}

	return 0;
}

static int pinmux_get(struct device *dev, uint32_t pin, uint32_t *func)
{
	volatile struct gpio_cmsdk_ahb *port = _get_port(pin);

	ARG_UNUSED(dev);

	if (!port) {
		return -EINVAL;
	}

	*func = (port->altfuncset & (1 << (pin % PINS_PER_PORT))) ? 1 : 0;

	return 0;
}

static int pinmux_pullup(struct device *dev, uint32_t pin, uint8_t func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	/* Beetle does not support programmable internal Pull-up/pull-down
	 * on IO Pads.
	 */
	return 0;
}

static int pinmux_input(struct device *dev, uint32_t pin, uint8_t func)
{
	volatile struct gpio_cmsdk_ahb *port = _get_port(pin);

	ARG_UNUSED(dev);

	if (!port) {
		return -EINVAL;
	}

	if (func) {
		port->outenableset = (1 << (pin % PINS_PER_PORT));
	} else {
		port->outenableclr = (1 << (pin % PINS_PER_PORT));
	}

	return 0;
}

static struct pinmux_driver_api api_funcs = {
	.set = pinmux_set,
	.get = pinmux_get,
	.pullup = pinmux_pullup,
	.input = pinmux_input
};

static int pinmux_dev_init(struct device *port)
{
	ARG_UNUSED(port);

	return 0;
}

DEVICE_AND_API_INIT(pmux_dev, CONFIG_PINMUX_DEV_NAME,
		&pinmux_dev_init, NULL, NULL,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&api_funcs);
