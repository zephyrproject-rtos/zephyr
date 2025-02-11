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

const struct xtensa_mmu_range xtensa_soc_mmu_ranges[] = {
	{
		.start = (uint32_t)XCHAL_VECBASE_RESET_VADDR,
		.end   = (uint32_t)CONFIG_SRAM_OFFSET,
		.attrs = XTENSA_MMU_PERM_X | XTENSA_MMU_CACHED_WB | XTENSA_MMU_MAP_SHARED,
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
		.attrs = XTENSA_MMU_PERM_X | XTENSA_MMU_CACHED_WB,
		.name = "rom",
	},
};

int xtensa_soc_mmu_ranges_num = ARRAY_SIZE(xtensa_soc_mmu_ranges);
