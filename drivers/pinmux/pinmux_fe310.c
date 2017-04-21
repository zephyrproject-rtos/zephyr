/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief PINMUX driver for the SiFive Freedom E310 Processor
 */

#include <errno.h>
#include <device.h>
#include <pinmux.h>
#include <soc.h>

struct pinmux_fe310_config {
	u32_t base;
};

struct pinmux_fe310_regs_t {
	u32_t iof_en;
	u32_t iof_sel;
};

#define DEV_CFG(dev)					\
	((const struct pinmux_fe310_config * const)	\
	 (dev)->config->config_info)

#define DEV_PINMUX(dev)						\
	((struct pinmux_fe310_regs_t *)(DEV_CFG(dev))->base)

static int pinmux_fe310_set(struct device *dev, u32_t pin, u32_t func)
{
	volatile struct pinmux_fe310_regs_t *pinmux = DEV_PINMUX(dev);

	if (func > FE310_PINMUX_IOF1 ||
	    pin >= FE310_PINMUX_PINS)
		return -EINVAL;

	if (func == FE310_PINMUX_IOF1)
		pinmux->iof_sel |= (FE310_PINMUX_IOF1 << pin);
	else
		pinmux->iof_sel &= ~(FE310_PINMUX_IOF1 << pin);

	/* Enable IO function for this pin */
	pinmux->iof_en |= (1 << pin);

	return 0;
}

static int pinmux_fe310_get(struct device *dev, u32_t pin, u32_t *func)
{
	volatile struct pinmux_fe310_regs_t *pinmux = DEV_PINMUX(dev);

	if (pin >= FE310_PINMUX_PINS ||
	    func == NULL)
		return -EINVAL;

	*func = (pinmux->iof_sel & (FE310_PINMUX_IOF1 << pin)) ?
		FE310_PINMUX_IOF1 : FE310_PINMUX_IOF0;

	return 0;
}

static int pinmux_fe310_pullup(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_fe310_input(struct device *dev, u32_t pin, u8_t func)
{
	return -ENOTSUP;
}

static int pinmux_fe310_init(struct device *dev)
{
	volatile struct pinmux_fe310_regs_t *pinmux = DEV_PINMUX(dev);

	/* Ensure that all pins are disabled initially */
	pinmux->iof_en = 0x0;

	return 0;
}

static const struct pinmux_driver_api pinmux_fe310_driver_api = {
	.set = pinmux_fe310_set,
	.get = pinmux_fe310_get,
	.pullup = pinmux_fe310_pullup,
	.input = pinmux_fe310_input,
};

static const struct pinmux_fe310_config pinmux_fe310_0_config = {
	.base = FE310_PINMUX_0_BASE_ADDR,
};

DEVICE_AND_API_INIT(pinmux_fe310_0, CONFIG_PINMUX_FE310_0_NAME,
		    &pinmux_fe310_init, NULL,
		    &pinmux_fe310_0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_fe310_driver_api);
