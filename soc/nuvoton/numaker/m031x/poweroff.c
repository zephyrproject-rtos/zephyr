/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
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

	/* Enter to Power-down mode */
	CLK_PowerDown();

	k_cpu_idle();

	CODE_UNREACHABLE;
}
