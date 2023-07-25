/*
 * Copyright (c) 2023 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>

#define DFSS_SPI0_BASE			0x80010000
#define DFSS_SPI1_BASE			0x80010100
#define REG_CLK_ENA_OFFSET		(0x16)  /* DFSS only */

/* Enable clock for DFSS SPI0 controller & DFSS SPI1 controller */
static int emsdp_dfss_clock_init(void)
{
	sys_out32(1, DFSS_SPI0_BASE + REG_CLK_ENA_OFFSET);
	sys_out32(1, DFSS_SPI1_BASE + REG_CLK_ENA_OFFSET);

	return 0;
}

SYS_INIT(emsdp_dfss_clock_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
