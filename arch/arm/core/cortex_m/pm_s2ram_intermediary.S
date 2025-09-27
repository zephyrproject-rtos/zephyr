/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M suspend-to-RAM resume intermediary code (S2RAM)
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/common/pm_s2ram.h>

GTEXT(pm_s2ram_mark_check_and_mediate)

GTEXT(arch_pm_s2ram_resume)
SECTION_FUNC(TEXT, arch_pm_s2ram_resume)
	/*
	 * Check if reset occurred after suspending to RAM.
	 * Store LR to ensure we can continue boot when we are not suspended
	 * to RAM. In addition to LR, R0 is pushed too, to ensure "SP mod 8 = 0",
	 * as stated by ARM rule 6.2.1.2 for AAPCS32.
	 */
	push    {r0, lr}
	bl      pm_s2ram_mark_check_and_mediate
	pop	{r0, pc}
