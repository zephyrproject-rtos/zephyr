/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/barrier.h>

void relocate_vector_table(void)
{
	/* Address zero is Boot ROM; use the linked ARM vectors in DRAM. */
	write_sctlr(read_sctlr() & ~(HIVECS | SCTLR_TE_Msk));
	write_vbar((uintptr_t)_vector_start & VBAR_MASK);
	barrier_isync_fence_full();
}
