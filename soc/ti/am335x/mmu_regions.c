/*
 * Copyright (c) 2024 Abe Kohandel <abe.kohandel@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/devicetree.h>

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("BOOTROM", 0x40000000, KB(176), MT_NORMAL | MPERM_R | MPERM_X),
	MMU_REGION_FLAT_ENTRY("SRAM", 0x402f0000, KB(128), MT_NORMAL | MPERM_R | MPERM_W | MPERM_X),
	MMU_REGION_FLAT_ENTRY("INTC", 0x48200000, KB(4), MT_DEVICE | MPERM_R | MPERM_W),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
