/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/poweroff.h>
#include <NuMicro.h>

void z_sys_poweroff(void)
{
	SYS_UnlockReg();

	/* Clear all wake-up flag */
	CLK->PMUSTS |= CLK_PMUSTS_CLRWK_Msk;

	/* Select Power-down mode */
	CLK_SetPowerDownMode(DT_PROP_OR(DT_NODELABEL(scc), powerdown_mode, CLK_PMUCTL_PDMSEL_SPD0));

	/* Enable RTC wake-up */
	CLK_ENABLE_RTCWK();

	/* Enter to Power-down mode */
	CLK_PowerDown();

	k_cpu_idle();

	CODE_UNREACHABLE;
}
