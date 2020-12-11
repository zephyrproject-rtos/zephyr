/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_s1000_pinmux

#include <kernel.h>
#include <drivers/pinmux.h>
#include <iomux.h>

static volatile uint32_t *iomux_ctrl_regs = (volatile uint32_t *)DT_INST_REG_ADDR(0);

#define PINMUX_CTRL_REG_COUNT (DT_INST_REG_SIZE(0) / 4)

static int pinmux_set(const struct device *dev, uint32_t pin, uint32_t func)
{
	uint32_t lsb, msb;
	uint32_t index;
	uint32_t value;
	uint32_t mask;

	/* retrieve control register index */
	index = IOMUX_INDEX(pin);

	/*
	 * retrieve the least and most significant bit positions for
	 * the pin group
	 */
	lsb = IOMUX_LSB(pin);
	msb = IOMUX_MSB(pin);

	if ((index >= PINMUX_CTRL_REG_COUNT) || (msb > 31) || (lsb > msb)) {
		return -EINVAL;
	}

	mask = BIT_MASK(msb - lsb + 1) << lsb;
	value = (func << lsb) & mask;

	iomux_ctrl_regs[index] = (iomux_ctrl_regs[index] & ~mask) | value;

	return 0;
}

static int pinmux_get(const struct device *dev, uint32_t pin, uint32_t *func)
{
	uint32_t lsb, msb;
	uint32_t index;
	uint32_t mask;

	/* retrieve control register index */
	index = IOMUX_INDEX(pin);

	/*
	 * retrieve the least and most significant bit positions for
	 * the pin group
	 */
	lsb = IOMUX_LSB(pin);
	msb = IOMUX_MSB(pin);

	if ((index >= PINMUX_CTRL_REG_COUNT) || (msb > 31) || (lsb > msb) ||
			(func == NULL)) {
		return -EINVAL;
	}

	mask = BIT_MASK(msb - lsb + 1);

	*func = (iomux_ctrl_regs[index] >> lsb) & mask;

	return 0;
}

static int pinmux_pullup(const struct device *dev, uint32_t pin, uint8_t func)
{
	return -ENOSYS;
}

static int pinmux_input(const struct device *dev, uint32_t pin, uint8_t func)
{
	return -ENOSYS;
}

static struct pinmux_driver_api apis = {
	.set = pinmux_set,
	.get = pinmux_get,
	.pullup = pinmux_pullup,
	.input = pinmux_input
};

static int pinmux_init(const struct device *device)
{
	ARG_UNUSED(device);
	return 0;
}

DEVICE_AND_API_INIT(pinmux, CONFIG_PINMUX_NAME, &pinmux_init, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY, &apis);
