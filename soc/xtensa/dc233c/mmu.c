/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <xtensa/config/core-isa.h>

#include <zephyr/devicetree.h>
#include <zephyr/arch/xtensa/xtensa_mmu.h>
#include <zephyr/sys/util.h>

#include "../../arch/xtensa/core/include/xtensa_mmu_priv.h"

const struct xtensa_mmu_range xtensa_soc_mmu_ranges[] = {
	{
		.start = (uint32_t)XCHAL_VECBASE_RESET_VADDR,
		.end   = (uint32_t)CONFIG_SRAM_OFFSET,
		.attrs = Z_XTENSA_MMU_X | Z_XTENSA_MMU_CACHED_WB,
		.name = "vecbase",
	},
	{
		/* The ROM is 32MB but the address wraps around back to 0x00000000.
		 * So just skip the last page so we don't have to deal with integer
		 * overflow.
		 */
		.start = (uint32_t)DT_REG_ADDR(DT_NODELABEL(rom0)),
		.end   = (uint32_t)DT_REG_ADDR(DT_NODELABEL(rom0)) +
			 (uint32_t)DT_REG_SIZE(DT_NODELABEL(rom0)),
		.attrs = Z_XTENSA_MMU_X | Z_XTENSA_MMU_CACHED_WB,
		.name = "rom",
	},
};

int xtensa_soc_mmu_ranges_num = ARRAY_SIZE(xtensa_soc_mmu_ranges);

void arch_xtensa_mmu_post_init(bool is_core0)
{
	uint32_t vecbase;

	ARG_UNUSED(is_core0);

	__asm__ volatile("rsr.vecbase %0" : "=r"(vecbase));

	/* Invalidate any autorefill instr TLBs of VECBASE so we can map it
	 * permanently below.
	 */
	xtensa_itlb_vaddr_invalidate((void *)vecbase);

	/* Map VECBASE permanently in instr TLB way 4 so we will always have
	 * access to exception handlers. Each way 4 TLB covers 1MB (unless
	 * ITLBCFG has been changed before this, which should not have
	 * happened).
	 *
	 * Note that we don't want to map the first 1MB in data TLB as
	 * we want to keep page 0 (0x00000000) unmapped to catch null pointer
	 * de-references.
	 */
	vecbase = ROUND_DOWN(vecbase, MB(1));
	xtensa_itlb_entry_write_sync(
		Z_XTENSA_PTE(vecbase, Z_XTENSA_KERNEL_RING,
			     Z_XTENSA_MMU_X | Z_XTENSA_MMU_CACHED_WT),
		Z_XTENSA_TLB_ENTRY((uint32_t)vecbase, 4));
}
