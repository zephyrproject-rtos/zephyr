/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <errno.h>
#include <sys/__assert.h>
#include <drivers/pinmux.h>

#include <driverlib/ioc.h>

static int pinmux_cc13xx_cc26xx_set(const struct device *dev, uint32_t pin,
				    uint32_t func)
{
	ARG_UNUSED(dev);

	__ASSERT_NO_MSG(pin < NUM_IO_MAX);
	__ASSERT_NO_MSG(func < NUM_IO_PORTS);

	IOCIOPortIdSet(pin, func);

	return 0;
}

static int pinmux_cc13xx_cc26xx_get(const struct device *dev, uint32_t pin,
				    uint32_t *func)
{
	ARG_UNUSED(dev);

	__ASSERT_NO_MSG(pin < NUM_IO_MAX);

	*func = IOCPortConfigureGet(pin) & IOC_IOCFG0_PORT_ID_M;

	return 0;
}

static int pinmux_cc13xx_cc26xx_pullup(const struct device *dev, uint32_t pin,
				       uint8_t func)
{
	ARG_UNUSED(dev);

	__ASSERT_NO_MSG(pin < NUM_IO_MAX);

	switch (func) {
	case PINMUX_PULLUP_ENABLE:
		IOCIOPortPullSet(pin, IOC_IOPULL_UP);
		return 0;
	case PINMUX_PULLUP_DISABLE:
		IOCIOPortPullSet(pin, IOC_NO_IOPULL);
		return 0;
	};

	return -EINVAL;
}

static int pinmux_cc13xx_cc26xx_input(const struct device *dev, uint32_t pin,
				      uint8_t func)
{
	ARG_UNUSED(dev);

	__ASSERT_NO_MSG(pin < NUM_IO_MAX);

	switch (func) {
	case PINMUX_INPUT_ENABLED:
		IOCIOInputSet(pin, IOC_INPUT_ENABLE);
		return 0;
	case PINMUX_OUTPUT_ENABLED:
		IOCIOInputSet(pin, IOC_INPUT_DISABLE);
		return 0;
	};

	return -EINVAL;
}

static int pinmux_cc13xx_cc26xx_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static const struct pinmux_driver_api pinmux_cc13xx_cc26xx_driver_api = {
	.set = pinmux_cc13xx_cc26xx_set,
	.get = pinmux_cc13xx_cc26xx_get,
	.pullup = pinmux_cc13xx_cc26xx_pullup,
	.input = pinmux_cc13xx_cc26xx_input,
};

DEVICE_AND_API_INIT(pinmux_cc13xx_cc26xx, CONFIG_PINMUX_NAME,
		    &pinmux_cc13xx_cc26xx_init, NULL, NULL, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_cc13xx_cc26xx_driver_api);
