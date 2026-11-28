/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>

#define GIC_NODE DT_NODELABEL(gic)
#define DEVICE_ATTRS (MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W)

static const struct arm_mmu_region mmu_regions[] = {
	/* ARMv7-A does not map the RAM vectors in the common MMU image ranges. */
	MMU_REGION_FLAT_ENTRY("vectors", (uintptr_t)_vector_start, CONFIG_MMU_PAGE_SIZE,
			      MT_NORMAL | MATTR_SHARED | MPERM_R | MPERM_X |
			      MATTR_CACHE_OUTER_WB_nWA | MATTR_CACHE_INNER_WB_nWA),

	MMU_REGION_FLAT_ENTRY("gic_dist", DT_REG_ADDR_BY_IDX(GIC_NODE, 0),
			      DT_REG_SIZE_BY_IDX(GIC_NODE, 0), DEVICE_ATTRS),
	MMU_REGION_FLAT_ENTRY("gic_cpu", DT_REG_ADDR_BY_IDX(GIC_NODE, 1),
			      DT_REG_SIZE_BY_IDX(GIC_NODE, 1), DEVICE_ATTRS),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
