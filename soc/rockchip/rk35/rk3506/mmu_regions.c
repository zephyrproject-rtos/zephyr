/*
 * Copyright (c) 2026 Jiaming Zhao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/sys/util.h>

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("gic", 0xff581000, 0x8000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
