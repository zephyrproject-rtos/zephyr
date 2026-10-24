/*
 * Copyright (C) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/init.h>
#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/kernel.h>

#define MMU_REGION_STRONG(cond, name) IF_ENABLED(cond,					\
		(MMU_REGION_FLAT_ENTRY(#name, DT_REG_ADDR(DT_NODELABEL(name)),		\
				       DT_REG_SIZE(DT_NODELABEL(name)),			\
				       MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),))

#define MMU_REGION_SFR_DEFN(name)							\
		MMU_REGION_STRONG(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(name)), name)

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("vectors", CONFIG_KERNEL_VM_BASE, 0x1000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_X),

	/* AIC Interrupt Redirection Register is located in SFR memory region */
	MMU_REGION_STRONG(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(aic)), sfr)

	MMU_REGION_SFR_DEFN(aic)
	MMU_REGION_SFR_DEFN(pmc)
	MMU_REGION_SFR_DEFN(rtc)
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
