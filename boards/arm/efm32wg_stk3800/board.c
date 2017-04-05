/*
 * Copyright (c) 2017 Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <board.h>
#include <gpio.h>
#include <misc/printk.h>

static int efm32wg_stk3800_init(struct device *dev)
{
	struct device *bce_dev; /* Board Controller Enable Gpio Device */

	ARG_UNUSED(dev);

	/* Enable the board controller to be able to use the serial port */
	bce_dev = device_get_binding(BC_ENABLE_GPIO_NAME);

	if (!bce_dev) {
		printk("Board controller gpio port was not found!\n");
		return -ENODEV;
	}

	gpio_pin_configure(bce_dev, BC_ENABLE_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(bce_dev, BC_ENABLE_GPIO_PIN, 1);

	return 0;
}

/* needs to be done after GPIO driver init */
SYS_INIT(efm32wg_stk3800_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
