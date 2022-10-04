/*
 * Copyright (c) 2022 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>

#define HSDK_CREG_GPIO_MUX_REG	0xf0001484
#define HSDK_CREG_GPIO_MUX_VAL	0x00000400

static int hsdk_creg_gpio_mux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	sys_write32(HSDK_CREG_GPIO_MUX_REG, HSDK_CREG_GPIO_MUX_VAL);

	return 0;
}

SYS_INIT(hsdk_creg_gpio_mux_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
