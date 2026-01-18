/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/arm_mmu.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#define DEVICE_ATTR (MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE)

static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("GIC",
			      DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 0),
			      DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 0),
			      DEVICE_ATTR),

	MMU_REGION_FLAT_ENTRY("GIC",
			      DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 1),
			      DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 1),
			      DEVICE_ATTR),

	MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY(amd_versal2_ufs, DEVICE_ATTR)

	MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY(xlnx_versal_8_9a, DEVICE_ATTR)

	MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY(amd_versal2_dma_1_0,
						DEVICE_ATTR)

	MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY(xlnx_versal_wwdt, DEVICE_ATTR)

	MMU_REGION_DT_COMPAT_FOREACH_FLAT_ENTRY(xlnx_mbox_versal_ipi_mailbox,
						DEVICE_ATTR)
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
