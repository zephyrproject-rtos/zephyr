/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/linker/linker-defs.h>

#include <cmsis_core.h>

void relocate_vector_table(void)
{
	size_t vector_size = (size_t)_vector_end - (size_t)_vector_start;
	uintptr_t addr = 0U;

	write_sctlr(read_sctlr() & ~HIVECS);
	(void)arch_early_memcpy((void *)addr, (void *)_vector_start, vector_size);

	/*
	 * Boot enables D/I-cache before z_arm_reset.  Without maintenance the
	 * relocated table at 0x0 may not be visible to instruction fetch, and
	 * prefetch-abort dispatch at 0xc faults with IFAR == 0xc.
	 */
	for (uintptr_t off = 0U; off < vector_size; off += 32U) {
		L1C_CleanInvalidateDCacheMVA((void *)(addr + off));
	}
	L1C_InvalidateICacheAll();
	__ISB();
}
