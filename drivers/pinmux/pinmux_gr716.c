/*
 * Copyright (c) 2021 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gaisler_gr716_pinctrl

#include <device.h>
#include <drivers/pinmux.h>
#include <dt-bindings/pinctrl/gr716-pinctrl.h>

struct regs {
	/** @brief System IO register for GPIO 0 to 7
	 *
	 * Bit    | Name       | Description
	 * ------ | ---------- | --------------------------------
	 * 31:28  | gpio7      | Functional select for GPIO 7
	 * 27:24  | gpio6      | Functional select for GPIO 6
	 * 23:20  | gpio5      | Functional select for GPIO 5
	 * 19:16  | gpio4      | Functional select for GPIO 4
	 * 15:12  | gpio3      | Functional select for GPIO 3
	 * 11:8   | gpio2      | Functional select for GPIO 2
	 *  7:4   | gpio1      | Functional select for GPIO 1
	 *  3:0   | gpio0      | Functional select for GPIO 0
	 */
	uint32_t gpio[8];

	/** @brief Pullup register for GPIO 0 to 31
	 *
	 * Bit    | Name       | Description
	 * ------ | ---------- | --------------------------------
	 * 31:0   | up         | Pullup config for GPIO 0 to 31
	 */
	uint32_t pullup[2];

	uint32_t pulldown[2];

	/**
	 * @brief IO configuration for LVDS
	 *
	 * Bit    | Name       | Description
	 * ------ | ---------- | --------------------------------
	 * 15:12  | rx0        | Select functionality for LVDS RX 0
	 * 11:8   | tx2        | Select functionality for LVDS TX 2
	 *  7:4   | tx1        | Select functionality for LVDS TX 1
	 *  3:0   | tx0        | Select functionality for LVDS TX 0
	 */
	uint32_t lvds;
};

static volatile struct regs *get_regs(void)
{
	return (struct regs *) DT_INST_REG_ADDR(0);
}

static void set_func(const struct device *dev, uint32_t pin, uint32_t func)
{
	/* There are 8 4-bit fields per control register. */
	const int regi  = pin / 8;
	const int field = pin % 8;
	const int shift = field * 4;
	const uint32_t mask = 0xf << shift;
	volatile struct regs *const regs = get_regs();
	uint32_t val;
	unsigned int key;

	key = irq_lock();
	val = regs->gpio[regi];
	val &= ~mask;
	val |= (func << shift);
	regs->gpio[regi] = val;
	irq_unlock(key);
}

static void set_pull(const struct device *dev, uint32_t pin, uint32_t func)
{
	/* There are 8 4-bit fields per control register. */
	const int regi  = pin / 32;
	const int field = pin % 32;
	const int shift = field * 1;
	const uint32_t mask = 0x1 << shift;
	uint32_t upmask = 0;
	uint32_t downmask = 0;
	volatile struct regs *const regs = get_regs();
	uint32_t val;
	unsigned int key;

	if (func == GR716_IO_PULL_DONTCHANGE) {
		return;
	} else if (func == GR716_IO_PULL_UP) {
		upmask = mask;
	} else if (func == GR716_IO_PULL_DOWN) {
		downmask = mask;
	} else if (func == GR716_IO_PULL_DISABLE) {
		;
	}

	key = irq_lock();

	val = regs->pullup[regi];
	val &= ~mask;
	val |= upmask;
	regs->pullup[regi] = val;

	val = regs->pulldown[regi];
	val &= ~mask;
	val |= downmask;
	regs->pulldown[regi] = val;

	irq_unlock(key);
}

static int set(const struct device *dev, uint32_t pin, uint32_t func)
{
	if (pin >= 64) {
		return -EINVAL;
	}
	if (GR716_IO_MODE_MAX < (func & GR716_IO_MODE_MASK)) {
		return -EINVAL;
	}

	set_func(dev, pin, func & GR716_IO_MODE_MASK);
	set_pull(dev, pin, func & GR716_IO_PULL_MASK);

	return 0;
}

static int get(const struct device *dev, uint32_t pin, uint32_t *func)
{
	const int regi  = pin / 8;
	const int field = pin % 8;
	const int shift = field * 4;
	volatile struct regs *const regs = get_regs();

	if (pin >= 64) {
		return -EINVAL;
	}

	*func = (regs->gpio[regi] >> shift) & 0xf;

	return 0;
}

static int pullup(const struct device *dev, uint32_t pin, uint8_t func)
{
	return -ENOTSUP;
}

static int input(const struct device *dev, uint32_t pin, uint8_t func)
{
	return -ENOTSUP;
}

static int init(const struct device *dev)
{
	return 0;
}

static const struct pinmux_driver_api api = {
	.set	= set,
	.get	= get,
	.pullup = pullup,
	.input	= input,
};

DEVICE_DT_INST_DEFINE(0,
		    &init, NULL, NULL,
		    NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api);
