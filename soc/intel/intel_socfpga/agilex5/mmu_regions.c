/*
 * Copyright (c) 2022, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm64/arm_mmu.h>

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("CLOCK",
			      DT_REG_ADDR(DT_NODELABEL(clock)),
			      DT_REG_SIZE(DT_NODELABEL(clock)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE),

	/* System manager register that required by clock driver */
	MMU_REGION_FLAT_ENTRY("SYSTEM_MANAGER",
			      DT_REG_ADDR(DT_NODELABEL(sysmgr)),
			      DT_REG_SIZE(DT_NODELABEL(sysmgr)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_DEFAULT_SECURE_STATE),

	MMU_REGION_FLAT_ENTRY("PINMUX",
			      DT_REG_ADDR_BY_IDX(DT_NODELABEL(pinmux), 0),
			      DT_REG_SIZE_BY_IDX(DT_NODELABEL(pinmux), 0),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE),

	MMU_REGION_FLAT_ENTRY("GIC_0",
			      DT_REG_ADDR_BY_IDX(DT_NODELABEL(gic), 0),
			      DT_REG_SIZE_BY_IDX(DT_NODELABEL(gic), 0),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_DEFAULT_SECURE_STATE),

	MMU_REGION_FLAT_ENTRY("GIC_1",
			      DT_REG_ADDR_BY_IDX(DT_NODELABEL(gic), 1),
			      DT_REG_SIZE_BY_IDX(DT_NODELABEL(gic), 1),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_DEFAULT_SECURE_STATE),

	MMU_REGION_FLAT_ENTRY("GIC_ITS",
			      DT_REG_ADDR(DT_NODELABEL(its)),
			      DT_REG_SIZE(DT_NODELABEL(its)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_DEFAULT_SECURE_STATE),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
