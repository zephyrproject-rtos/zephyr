/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
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

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hxt)
	/* Enable/disable 4~24 MHz external crystal oscillator (HXT) */
	if (DT_ENUM_IDX(DT_NODELABEL(scc), hxt) == NUMAKER_SCC_CLKSW_ENABLE) {
		CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
		/* Wait for HXT clock ready */
		CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
	} else if (DT_ENUM_IDX(DT_NODELABEL(scc), hxt) == NUMAKER_SCC_CLKSW_DISABLE) {
		CLK_DisableXtalRC(CLK_PWRCTL_HXTEN_Msk);
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), lxt)
	/* Enable/disable 32.768 kHz low-speed external crystal oscillator (LXT) */
	if (DT_ENUM_IDX(DT_NODELABEL(scc), lxt) == NUMAKER_SCC_CLKSW_ENABLE) {
		CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
		/* Wait for LXT clock ready */
		CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);
	} else if (DT_ENUM_IDX(DT_NODELABEL(scc), lxt) == NUMAKER_SCC_CLKSW_DISABLE) {
		CLK_DisableXtalRC(CLK_PWRCTL_LXTEN_Msk);
	}
#endif

	/* Enable 12 MHz high-speed internal RC oscillator (HIRC) */
	CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
	/* Wait for HIRC clock ready */
	CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

	/* Enable 10 KHz low-speed internal RC oscillator (LIRC) */
	CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
	/* Wait for LIRC clock ready */
	CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), hirc48)
	/* Enable/disable 48 MHz high-speed internal RC oscillator (HIRC48) */
	if (DT_ENUM_IDX(DT_NODELABEL(scc), hirc48) == NUMAKER_SCC_CLKSW_ENABLE) {
		CLK_EnableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);
		/* Wait for HIRC48 clock ready */
		CLK_WaitClockReady(CLK_STATUS_HIRC48STB_Msk);
	} else if (DT_ENUM_IDX(DT_NODELABEL(scc), hirc48) == NUMAKER_SCC_CLKSW_DISABLE) {
		CLK_DisableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);
	}
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), clk_pclkdiv)
	/* Set CLK_PCLKDIV register on request */
	CLK->PCLKDIV = DT_PROP(DT_NODELABEL(scc), clk_pclkdiv);
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(scc), core_clock)
	/* Set core clock (HCLK) on request */
	CLK_SetCoreClock(DT_PROP(DT_NODELABEL(scc), core_clock));
#endif

	/*
	 * Update System Core Clock
	 * User can use SystemCoreClockUpdate() to calculate SystemCoreClock.
	 */
	SystemCoreClockUpdate();

	/* Lock protected registers */
	SYS_LockReg();
}
