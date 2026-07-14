/*
 * Copyright (C) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/init.h>
#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/kernel.h>

#define MMU_REGION_STRONG(cond, name, base, size) IF_ENABLED(cond,			\
		(MMU_REGION_FLAT_ENTRY(name, base, size,				\
				       MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),))

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("vectors", CONFIG_KERNEL_VM_BASE, 0x1000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_X),

	MMU_REGION_STRONG(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aic)), "sfr",
			  SFR_BASE_ADDRESS, 0xa0)

	MMU_REGION_STRONG(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aic)), "aic",
			  AIC_BASE_ADDRESS, 0x100)

	MMU_REGION_STRONG(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(PMC)), "pmc",
			  PMC_BASE_ADDRESS, 0x200)
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};

void relocate_vector_table(void)
{
	write_vbar(CONFIG_KERNEL_VM_BASE);
}

void soc_early_init_hook(void)
{
	/* Apply SoC related preinit configuration */
}
