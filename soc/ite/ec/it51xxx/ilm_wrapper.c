/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ilm.h>
#include <soc_common.h>
#include <zephyr/kernel.h>

void __soc_ram_code custom_reset_instr_cache(void)
{
	struct gctrl_it51xxx_regs *const gctrl_regs = GCTRL_IT51XXX_REGS_BASE;

	/* I-Cache tag sram reset */
	gctrl_regs->GCTRL_SCR0BAR = 0;
	/* Make sure the I-Cache is reset */
	__asm__ volatile("fence.i" ::: "memory");
}
