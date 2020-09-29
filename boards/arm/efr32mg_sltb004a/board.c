/*
 * Copyright (c) 2020 Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include "board.h"
#include <drivers/gpio.h>
#include <sys/printk.h>

static int efr32mg_sltb004a_init(const struct device *dev)
{
	const struct device *cur_dev;

	ARG_UNUSED(dev);

#ifdef CONFIG_CCS811
	/* Enable the CCS811 power */
	cur_dev = device_get_binding(CCS811_PWR_ENABLE_GPIO_NAME);
	if (!cur_dev) {
		printk("CCS811 power gpio port was not found!\n");
		return -ENODEV;
	}

	gpio_pin_configure(cur_dev, CCS811_PWR_ENABLE_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_set(cur_dev, CCS811_PWR_ENABLE_GPIO_PIN, 1);

#endif /* CONFIG_CCS811 */

	return 0;
}

/* needs to be done after GPIO driver init */
SYS_INIT(efr32mg_sltb004a_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
