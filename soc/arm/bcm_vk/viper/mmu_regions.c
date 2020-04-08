/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <sys/util.h>
#include <arch/arm/aarch64/arm_mmu.h>

static const struct arm_mmu_region mmu_regions[] = {

	MMU_REGION_FLAT_ENTRY("DEVICE_REGION",
			      0x40000000, MB(512),
			      MT_DEVICE_nGnRnE | MT_RW | MT_SECURE),

	MMU_REGION_FLAT_ENTRY("DRAM0_S0",
			      0x60000000, MB(512),
			      MT_NORMAL | MT_RW | MT_SECURE),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
