/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief
 *
 * A common driver for STM32 pinmux. Each SoC must implement a SoC
 * specific part of the driver.
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include "pinmux.h"
#include <pinmux.h>
#include <pinmux/stm32/pinmux_stm32.h>

static int pinmux_stm32_set(struct device *dev,
				 uint32_t pin, uint32_t func)
{
	ARG_UNUSED(dev);

	return _pinmux_stm32_set(pin, func, NULL);
}

static int pinmux_stm32_get(struct device *dev,
				 uint32_t pin, uint32_t *func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	return -ENOTSUP;
}

static int pinmux_stm32_input(struct device *dev,
				uint32_t pin,
				uint8_t func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	return -ENOTSUP;
}

static int pinmux_stm32_pullup(struct device *dev,
				uint32_t pin,
				uint8_t func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	return -ENOTSUP;
}

static const struct pinmux_driver_api pinmux_stm32_api = {
	.set = pinmux_stm32_set,
	.get = pinmux_stm32_get,
	.pullup = pinmux_stm32_pullup,
	.input = pinmux_stm32_input,
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	return 0;
}

DEVICE_AND_API_INIT(pmux_dev, CONFIG_PINMUX_DEV_NAME, &pinmux_stm32_init,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY,
		    &pinmux_stm32_api);
