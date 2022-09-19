/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 */

#include <xmc_scu.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <soc.h>

#define PMU_FLASH_WS		(0x3U)

void z_arm_platform_init(void)
{
	uint32_t temp;

	/* setup flash wait state */
	temp = FLASH0->FCON;
	temp &= ~FLASH_FCON_WSPFLASH_Msk;
	temp |= PMU_FLASH_WS;
	FLASH0->FCON = temp;

	XMC_SCU_CLOCK_SetSleepConfig(XMC_SCU_CLOCK_SLEEP_MODE_CONFIG_SYSCLK_FPLL);

	/* configure PLL & system clock */
	SystemCoreClockSetup();
}
