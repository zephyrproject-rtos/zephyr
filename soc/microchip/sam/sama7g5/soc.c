/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/init.h>
#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/kernel.h>

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("vectors", CONFIG_KERNEL_VM_BASE, 0x1000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_X),

	MMU_REGION_FLAT_ENTRY("flexcom3", FLEXCOM3_BASE_ADDRESS, 0x4000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),

	MMU_REGION_FLAT_ENTRY("gic", GIC_DISTRIBUTOR_BASE, 0x1100,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),

	MMU_REGION_FLAT_ENTRY("pioa", PIO_BASE_ADDRESS, 0x4000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),

	MMU_REGION_FLAT_ENTRY("pit64b0", PIT64B0_BASE_ADDRESS, 0x4000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),

	MMU_REGION_FLAT_ENTRY("pmc", PMC_BASE_ADDRESS, 0x200,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),

	MMU_REGION_FLAT_ENTRY("sckc", SCKC_BASE_ADDRESS, 0x4,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),
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
	/* Enable Generic clock for PIT64B0 for system tick */
	PMC_REGS->PMC_PCR = PMC_PCR_CMD(1) | PMC_PCR_GCLKEN(1) | PMC_PCR_EN(1) |
			    PMC_PCR_GCLKDIV(40 - 1) | PMC_PCR_GCLKCSS_SYSPLL |
			    PMC_PCR_PID(ID_PIT64B0);
}
