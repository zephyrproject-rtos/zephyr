/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 * Copyright (c) 2020 Thorvald Natvig
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "em_cmu.h"
#include "board.h"

static int efm32gg_slwstk6121a_init(void)
{

	const struct device *cur_dev;

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
	CMU->CTRL |= CMU_CTRL_CLKOUTSEL2_HFXO;
	CMU->ROUTELOC0 = (CMU->ROUTELOC0 & ~_CMU_ROUTELOC0_CLKOUT2LOC_MASK) |
			 (ETH_REF_CLK_LOCATION << _CMU_ROUTELOC0_CLKOUT2LOC_SHIFT);
	CMU->ROUTEPEN |= CMU_ROUTEPEN_CLKOUT2PEN;

	return 0;
}

/* needs to be done after GPIO driver init and device tree available */
SYS_INIT(efm32gg_slwstk6121a_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
