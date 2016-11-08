/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <nanokernel.h>
#include <board.h>

#include <device.h>
#include <errno.h>
#include <init.h>
#include <pinmux.h>
#include <soc.h>
#include <misc/util.h>

static volatile struct __pio *_get_port(uint32_t pin)
{
	uint32_t port_num = pin / 32;

	switch (port_num) {
	case 0:
		return __PIOA;
	case 1:
		return __PIOB;
	case 2:
		return __PIOC;
	case 3:
		return __PIOD;
	default:
		/* return null if pin is outside range */
		return NULL;
	}
}

static int pinmux_set(struct device *dev, uint32_t pin, uint32_t func)
{
	volatile struct __pio *port = _get_port(pin);
	uint32_t tmp;

	ARG_UNUSED(dev);

	if (!port) {
		return -EINVAL;
	}

	tmp = port->absr;
	if (func) {
		tmp |= (1 << (pin % 32));
	} else {
		tmp &= ~(1 << (pin % 32));
	}
	port->absr = tmp;

	return 0;
}

static int pinmux_get(struct device *dev, uint32_t pin, uint32_t *func)
{
	volatile struct __pio *port = _get_port(pin);

	ARG_UNUSED(dev);

	if (!port) {
		return -EINVAL;
	}

	*func = (port->absr & (1 << (pin % 32))) ? 1 : 0;

	return 0;
}

static int pinmux_pullup(struct device *dev, uint32_t pin, uint8_t func)
{
	volatile struct __pio *port = _get_port(pin);

	ARG_UNUSED(dev);

	if (!port) {
		return -EINVAL;
	}

	if (func) {
		port->puer = (1 << (pin % 32));
	} else {
		port->pudr = (1 << (pin % 32));
	}

	return 0;
}
static int pinmux_input(struct device *dev, uint32_t pin, uint8_t func)
{
	volatile struct __pio *port = _get_port(pin);

	ARG_UNUSED(dev);

	if (!port) {
		return -EINVAL;
	}

	if (func) {
		port->odr = (1 << (pin % 32));
	} else {
		port->oer = (1 << (pin % 32));
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
	return 0;
}

DEVICE_AND_API_INIT(pmux_dev, CONFIG_PINMUX_DEV_NAME,
		    &pinmux_dev_init, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api_funcs);
