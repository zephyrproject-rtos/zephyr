/*
 * Copyright (c) 2021 Jim Shu
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <linker/linker-defs.h>

#include "../../../arch/riscv/include/core_pmp.h"
#include "../../common/riscv_pmp_mem_cfg.h"

#if defined(CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_PMP_REGIONS) && defined(CONFIG_XIP)
static const struct riscv_pmp_region pmp_regions[] = {
	PMP_REGION_ENTRY("FLASH_0",
			 0x20000000,
			 REGION_FLASH_SIZE,
			 REGION_ROM_ATTR),
};
#endif

const struct riscv_pmp_config pmp_config = {
	.num_regions = ARRAY_SIZE(pmp_regions),
	.pmp_regions = pmp_regions,
};
