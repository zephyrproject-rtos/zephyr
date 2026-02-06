/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Linumiz
 * Author: Saravanan Sekar <saravanan@linumiz.com>
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>

void soc_reset_hook(void)
{
	SYS_UnlockReg();

	/* Disable SPIM cache to relocate 32 KB to SRAM */
	CLK->AHBCLK |= CLK_AHBCLK_SPIMCKEN_Msk;
	SPIM->CTL1 |= SPIM_CTL1_CACHEOFF_Msk;
	SPIM->CTL1 |= SPIM_CTL1_CCMEN_Msk;

	/* Set flash access time to 8, later real value set in clock driver */
	FMC->CYCCTL = 8;

	SYS_LockReg();
}
