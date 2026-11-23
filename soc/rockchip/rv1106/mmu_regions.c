/*
 * Copyright (c) 2026 Patryk Koscik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

#include <zephyr/arch/arm/mmu/arm_mmu.h>

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("vectors", 0, 0x1000, MT_STRONGLY_ORDERED | MPERM_R | MPERM_X),
	MMU_REGION_FLAT_ENTRY("gic", 0xff1f1000, 0x3000, MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),
	MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY(ns16550, MT_DEVICE | MPERM_R | MPERM_W)};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
