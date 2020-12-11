/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <devicetree.h>
#include <soc.h>
#include <sys/util.h>
#include <arch/arm/aarch64/arm_mmu.h>


#define PCIE_OB_HIGHMEM_ADDR	DT_REG_ADDR_BY_NAME(DT_NODELABEL(pcie0_ep), \
						    map_highmem)
#define PCIE_OB_HIGHMEM_SIZE	DT_REG_SIZE_BY_NAME(DT_NODELABEL(pcie0_ep), \
						    map_highmem)

static const struct arm_mmu_region mmu_regions[] = {

	MMU_REGION_FLAT_ENTRY("DEVICE_REGION",
			      0x40000000, MB(512),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_SECURE),

	MMU_REGION_FLAT_ENTRY("PCIE_HIGH_OBMEM",
			      PCIE_OB_HIGHMEM_ADDR, PCIE_OB_HIGHMEM_SIZE,
			      MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_SECURE),

	MMU_REGION_FLAT_ENTRY("DRAM0_S0",
			      0x60000000, MB(512),
			      MT_NORMAL | MT_P_RW_U_NA | MT_SECURE),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
