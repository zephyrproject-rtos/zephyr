/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
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

	/* Select Power-down mode */
	PMC_SetPowerDownMode(DT_PROP_OR(DT_NODELABEL(scc), powerdown_mode, PMC_SPD0),
			     PMC_PLCTL_PLSEL_PL0);

	/* Clear all wake-up flag */
	PMC->INTSTS |= PMC_INTSTS_CLRWK_Msk;

	/* Enter to Power-down mode */
	PMC_PowerDown();

	k_cpu_idle();

	CODE_UNREACHABLE;
}
