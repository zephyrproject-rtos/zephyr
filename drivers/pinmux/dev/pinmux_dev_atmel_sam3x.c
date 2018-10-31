/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>

#include <device.h>
#include <errno.h>
#include <init.h>
#include <pinmux.h>
#include <soc.h>
#include <misc/util.h>

static Pio *_get_port(u32_t pin)
{
	u32_t port_num = pin / 32;

	switch (port_num) {
	case 0:
		return PIOA;
	case 1:
		return PIOB;
	case 2:
		return PIOC;
	case 3:
		return PIOD;
	default:
		/* return null if pin is outside range */
		return NULL;
	}
}

static int pinmux_set(struct device *dev, u32_t pin, u32_t func)
{
	Pio *port = _get_port(pin);
	u32_t tmp;

	ARG_UNUSED(dev);

	if (!port) {
		return -EINVAL;
	}

	tmp = port->PIO_ABSR;
	if (func) {
		tmp |= (1 << (pin % 32));
	} else {
		tmp &= ~(1 << (pin % 32));
	}
	port->PIO_ABSR = tmp;

	return 0;
}

static int pinmux_get(struct device *dev, u32_t pin, u32_t *func)
{
	Pio *port = _get_port(pin);

	ARG_UNUSED(dev);

	if (!port) {
		return -EINVAL;
	}

	*func = (port->PIO_ABSR & (1 << (pin % 32))) ? 1 : 0;

	return 0;
}

static int pinmux_pullup(struct device *dev, u32_t pin, u8_t func)
{
	Pio *port = _get_port(pin);

	ARG_UNUSED(dev);

	if (!port) {
		return -EINVAL;
	}

	if (func) {
		port->PIO_PUER = (1 << (pin % 32));
	} else {
		port->PIO_PUDR = (1 << (pin % 32));
	}

	return 0;
}
static int pinmux_input(struct device *dev, u32_t pin, u8_t func)
{
	Pio *port = _get_port(pin);

	ARG_UNUSED(dev);

	if (!port) {
		return -EINVAL;
	}

	if (func) {
		port->PIO_ODR = (1 << (pin % 32));
	} else {
		port->PIO_OER = (1 << (pin % 32));
	}

	return 0;
}

static const struct pinmux_driver_api api_funcs = {
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
