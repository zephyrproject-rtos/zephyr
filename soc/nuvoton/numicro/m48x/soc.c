/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Linumiz
 * Author: Saravanan Sekar <saravanan@linumiz.com>
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>

void z_arm_platform_init(void)
{
	SYS_UnlockReg();

	/* system clock init */
	SystemInit();

	/* Enable HXT clock (external XTAL 12MHz) */
	CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

	/* Wait for HXT clock ready */
	CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

	/* Set core clock as PLL_FOUT source */
	CLK_SetCoreClock(FREQ_192MHZ);

	/* Set both PCLK0 and PCLK1 as HCLK/2 */
	CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

	SystemCoreClockUpdate();

	SYS_LockReg();
}
