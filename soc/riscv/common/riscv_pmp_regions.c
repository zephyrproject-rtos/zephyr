/*
 * Copyright (c) 2021 Jim Shu
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <linker/linker-defs.h>

#include "../../../arch/riscv/include/core_pmp.h"
#include "riscv_pmp_mem_cfg.h"

#ifndef CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_PMP_REGIONS
#ifdef CONFIG_XIP
static const struct riscv_pmp_region pmp_regions[] = {
	PMP_REGION_ENTRY("FLASH_0",
			 FLASH_BASE,
			 REGION_FLASH_SIZE,
			 REGION_ROM_ATTR),
};
#else /* CONFIG_XIP */
static const struct riscv_pmp_region pmp_regions[] = {
	PMP_REGION_ENTRY("ROM",
			 (ulong_t) __rom_region_start,
			 (ulong_t) __rom_region_size,
			 REGION_ROM_ATTR),
};
#endif /* CONFIG_XIP */

const struct riscv_pmp_config pmp_config = {
	.num_regions = ARRAY_SIZE(pmp_regions),
	.pmp_regions = pmp_regions,
};
#endif /* !CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_PMP_REGIONS */
