/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/arm_mmu.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

static const struct arm_mmu_region mmu_regions[] = {

	MMU_REGION_FLAT_ENTRY("GIC",
			      DT_REG_ADDR_BY_IDX(DT_NODELABEL(gic), 0),
			      DT_REG_SIZE_BY_IDX(DT_NODELABEL(gic), 0),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("GIC",
			      DT_REG_ADDR_BY_IDX(DT_NODELABEL(gic), 1),
			      DT_REG_SIZE_BY_IDX(DT_NODELABEL(gic), 1),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("CCM",
			      DT_REG_ADDR(DT_NODELABEL(ccm)),
			      DT_REG_SIZE(DT_NODELABEL(ccm)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("UART2",
			      DT_REG_ADDR(DT_NODELABEL(uart2)),
			      DT_REG_SIZE(DT_NODELABEL(uart2)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("ANA_PLL",
			      DT_REG_ADDR(DT_NODELABEL(ana_pll)),
			      DT_REG_SIZE(DT_NODELABEL(ana_pll)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("UART4",
			      DT_REG_ADDR(DT_NODELABEL(uart4)),
			      DT_REG_SIZE(DT_NODELABEL(uart4)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),
	/* TODO: Add more device entry */
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
