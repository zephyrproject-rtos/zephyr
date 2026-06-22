/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include "board.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "em_cmu.h"

static int efm32gg_stk3701a_init(void)
{
#ifdef CONFIG_ETH_GECKO
	const struct device *cur_dev;

	/* Enable the ethernet PHY power */
	cur_dev = DEVICE_DT_GET(ETH_PWR_ENABLE_GPIO_NODE);
	if (!device_is_ready(cur_dev)) {
		printk("Ethernet PHY power gpio port is not ready!\n");
		return -ENODEV;
	}

	gpio_pin_configure(cur_dev, ETH_PWR_ENABLE_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_set(cur_dev, ETH_PWR_ENABLE_GPIO_PIN, 1);

	/* Configure ethernet reference clock */
	cur_dev = DEVICE_DT_GET(ETH_REF_CLK_GPIO_NODE);
	if (!device_is_ready(cur_dev)) {
		printk("Ethernet reference clock gpio port is not ready!\n");
		return -ENODEV;
	}

	gpio_pin_configure(cur_dev, ETH_REF_CLK_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_set(cur_dev, ETH_REF_CLK_GPIO_PIN, 0);

	CMU_OscillatorEnable(cmuOsc_HFXO, true, true);

	/* enable CMU_CLK2 as RMII reference clock */
	CMU->CTRL      |= CMU_CTRL_CLKOUTSEL2_HFXO;
	CMU->ROUTELOC0 = (CMU->ROUTELOC0 & ~_CMU_ROUTELOC0_CLKOUT2LOC_MASK) |
		(ETH_REF_CLK_LOCATION << _CMU_ROUTELOC0_CLKOUT2LOC_SHIFT);
	CMU->ROUTEPEN  |= CMU_ROUTEPEN_CLKOUT2PEN;

	/* Release the ethernet PHY reset */
	cur_dev = DEVICE_DT_GET(ETH_RESET_GPIO_NODE);
	if (!device_is_ready(cur_dev)) {
		printk("Ethernet PHY reset gpio port is not ready!\n");
		return -ENODEV;
	}

	gpio_pin_configure(cur_dev, ETH_RESET_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_set(cur_dev, ETH_RESET_GPIO_PIN, 1);
#endif /* CONFIG_ETH_GECKO */

	return 0;
}

/* needs to be done after GPIO driver init */
SYS_INIT(efm32gg_stk3701a_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
