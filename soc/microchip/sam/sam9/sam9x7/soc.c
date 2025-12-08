/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <soc.h>
#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#define MMU_REGION_SFR_DEFN(name, base, size)							\
		IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(name)),				\
			   (MMU_REGION_FLAT_ENTRY(#name, base, size,				\
						  MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),))

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_ENTRY("vectors", CONFIG_KERNEL_VM_BASE, 0, 0x1000,
			 MT_STRONGLY_ORDERED | MPERM_R | MPERM_X),

	MMU_REGION_SFR_DEFN(aic, AIC_BASE_ADDRESS, 0x100)
	MMU_REGION_SFR_DEFN(dbgu, DBGU_BASE_ADDRESS, 0x100)
	MMU_REGION_SFR_DEFN(pit64b0, PIT64B0_BASE_ADDRESS, 0x100)
	MMU_REGION_SFR_DEFN(pmc, PMC_BASE_ADDRESS, 0x100)
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};

void soc_early_init_hook(void)
{
}
