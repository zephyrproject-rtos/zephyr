/*
 * Copyright (c) 2025 Syswonder
 * SPDX-License-Identifier: Apache-2.0
 */
#include "zephyr/linker/linker-defs.h"
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <mmu.h>

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("vector", (uintptr_t)_vector_start, 0x1000,
			      MT_NORMAL | MATTR_SHARED | MPERM_R | MPERM_X),
	MMU_REGION_FLAT_ENTRY("GIC", DT_REG_ADDR_BY_IDX(DT_NODELABEL(gic), 0),
			      DT_REG_SIZE_BY_IDX(DT_NODELABEL(gic), 0),
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),

	MMU_REGION_FLAT_ENTRY("GIC", DT_REG_ADDR_BY_IDX(DT_NODELABEL(gic), 1),
			      DT_REG_SIZE_BY_IDX(DT_NODELABEL(gic), 1),
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
