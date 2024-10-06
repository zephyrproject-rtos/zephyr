/*
 * Copyright (c) 2022 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>

#define HSDK_CREG_GPIO_MUX_REG	0xf0001484
#define HSDK_CREG_GPIO_MUX_VAL	0x00000400

void board_early_init_hook(void)
{
	sys_write32(HSDK_CREG_GPIO_MUX_REG, HSDK_CREG_GPIO_MUX_VAL);

}
