/*
 * Copyright (c) 2023 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>

#define DFSS_SPI0_BASE			0x80010000
#define DFSS_SPI1_BASE			0x80010100
#define SPI_REG_CLK_ENA_OFFSET		(0x16)  /* DFSS only */
#define DFSS_I2C0_BASE			0x80012000
#define DFSS_I2C1_BASE			0x80012100
#define DFSS_I2C2_BASE			0x80012200
#define I2C_REG_CLK_ENA_OFFSET		(0xC0)  /* DFSS only */

/* Enable clock for DFSS SPI and I2C controller */
static int emsdp_dfss_clock_init(void)
{
	sys_out32(1, DFSS_SPI0_BASE + SPI_REG_CLK_ENA_OFFSET);
	sys_out32(1, DFSS_SPI1_BASE + SPI_REG_CLK_ENA_OFFSET);
	sys_out32(1, DFSS_I2C0_BASE + I2C_REG_CLK_ENA_OFFSET);
	sys_out32(1, DFSS_I2C1_BASE + I2C_REG_CLK_ENA_OFFSET);
	sys_out32(1, DFSS_I2C2_BASE + I2C_REG_CLK_ENA_OFFSET);
	return 0;
}

SYS_INIT(emsdp_dfss_clock_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
