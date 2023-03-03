/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
/* Hardware and starter kit includes. */
#include <NuMicro.h>

void z_arm_platform_init(void)
{
	SystemInit();

	/* Unlock protected registers */
	SYS_UnlockReg();

	/*
	 * -------------------
	 * Init System Clock
	 * -------------------
	 */

	CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
	/* Wait for HXT clock ready */
	CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

	CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
	/* Wait for LXT clock ready */
	CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

	/* Enable 12 MHz high-speed internal RC oscillator (HIRC) */
	CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
	/* Wait for HIRC clock ready */
	CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

	/* Enable 10 KHz low-speed internal RC oscillator (LIRC) */
	CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
	/* Wait for LIRC clock ready */
	CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

	CLK_EnableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);
	/* Wait for HIRC48 clock ready */
	CLK_WaitClockReady(CLK_STATUS_HIRC48STB_Msk);

	/* Set PCLK0 and PCLK1 to HCLK/2 */
	CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

	/* Set core clock to 200MHz */
	CLK_SetCoreClock(200000000);

	/*
	 * Update System Core Clock
	 * User can use SystemCoreClockUpdate() to calculate SystemCoreClock.
	 */
	SystemCoreClockUpdate();

	/* Lock protected registers */
	SYS_LockReg();
}
