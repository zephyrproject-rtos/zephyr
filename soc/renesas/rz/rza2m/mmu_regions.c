/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/devicetree.h>

extern char _vector_start[];

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("vector_tables", POINTER_TO_UINT(_vector_start), 0x1000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_X),

	MMU_REGION_FLAT_ENTRY("pl310", 0x1F003000, 0x1000,
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),

	MMU_REGION_FLAT_ENTRY("gic", DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 0),
			      DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 0),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),

	MMU_REGION_FLAT_ENTRY("gic", DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 1),
			      DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 1),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
