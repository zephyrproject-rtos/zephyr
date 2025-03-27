/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/arm_mmu.h>
#include <zephyr/devicetree.h>

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("IO_REG", 0x10000000, 0x10000000,
			      MT_DEVICE_nGnRnE | MT_RW | MT_DEFAULT_SECURE_STATE),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
