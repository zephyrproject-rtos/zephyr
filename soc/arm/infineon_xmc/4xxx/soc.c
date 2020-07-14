/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>

#define PMU_FLASH_WS		(0x3U)

void z_platform_init(void)
{
	uint32_t temp;

	/* setup flash wait state */
	temp = FLASH0->FCON;
	temp &= ~FLASH_FCON_WSPFLASH_Msk;
	temp |= PMU_FLASH_WS;
	FLASH0->FCON = temp;

	/* configure PLL & system clock */
	SystemCoreClockSetup();
}
