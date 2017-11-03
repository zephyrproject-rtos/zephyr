/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

#define IOMUX_BASE		0x00081C00
#define IOMUX_CONTROL0		(IOMUX_BASE + 0x30)
#define IOMUX_CONTROL2		(IOMUX_BASE + 0x38)
#define TS_POWER_CONFIG		0x00071F90

/* This semaphore is used to serialize the UART prints dumped by various
 * modules. This prevents mixing of UART prints across modules. This
 * semaphore starts off "available".
 */
K_SEM_DEFINE(thread_sem, 1, 1);

/* Disable Tensilica power gating */
void disable_ts_powergate(void)
{
	volatile u16_t pwrcfg = *(volatile u16_t *)TS_POWER_CONFIG;

	/* Set the below bits to disable power gating:
	 * BIT0 - Tensilica Core Prevent DSP Core Power Gating
	 * BIT4 - Tensilica Core Prevent Controller Power Gating
	 * BIT5 - Ignore D3 / D0i3 Power Gating
	 * BIT6 - Tensilica Core Prevent DSP Common Power Gating
	 */
	pwrcfg |= BIT(0) | BIT(4) | BIT(5) | BIT(6);

	*(volatile u16_t *)TS_POWER_CONFIG = pwrcfg;
}

/* Configure the MUX to select GPIO functionality for GPIO 23 and 24 */
void iomux_config_ctsrts(void)
{
	volatile u32_t iomux_cntrl0 = *(volatile u32_t *)IOMUX_CONTROL0;

	/* Set bit 16 to convert the pins to normal GPIOs from UART_RTS_CTS */
	iomux_cntrl0 |= BIT(16);

	*(volatile u32_t *)IOMUX_CONTROL0 = iomux_cntrl0;
}

/* Configure the MUX to select the correct I2C port (I2C1) */
void iomux_config_i2c(void)
{
	volatile u32_t iomux_cntrl2 = *(volatile u32_t *)IOMUX_CONTROL2;

	/* Set bit 0 to select i2c1 */
	iomux_cntrl2 |= BIT(0);

	*(volatile u32_t *)IOMUX_CONTROL2 = iomux_cntrl2;
}

void main(void)
{
	printk("Sample app running on: %s Intel_S1000\n", CONFIG_ARCH);

	disable_ts_powergate();
	iomux_config_i2c();
	iomux_config_ctsrts();
}
