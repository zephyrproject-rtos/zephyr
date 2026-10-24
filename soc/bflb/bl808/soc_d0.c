/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bouffalo Lab RISC-V MCU series initialization code
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#include "thead_csr.h"

void soc_early_init_hook(void)
{
	/* Invalidate all MMU TLB Entries */
	__asm__ volatile("sfence.vma x0, x0");
}

void arch_cache_init(void)
{
	/* MXSTATUS is 32-bits on C906, so using the same code as E907 is fine */
	enable_thead_isa_ext();
	set_thead_enforce_aligned(false);
	sys_cache_data_enable();
	enable_branchpred(true);
	sys_cache_instr_enable();
	disable_interrupt_autostacking();
	set_thead_mhint(true, AMR_64L, PREF_N_2);
	sys_cache_data_flush_and_invd_all();
	sys_cache_instr_invd_all();
}
