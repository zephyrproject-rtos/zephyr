/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

static int soc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (IS_ENABLED(CONFIG_SOC_MEC172X_TEST_CLK_OUT)) {

		struct gpio_regs * const regs =
			(struct gpio_regs * const)DT_REG_ADDR(DT_NODELABEL(pinctrl));

		regs->CTRL[MCHP_GPIO_0060_ID] = MCHP_GPIO_CTRL_MUX_F2 |
						MCHP_GPIO_CTRL_IDET_DISABLE;
	}

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
