/*
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/devicetree.h>

/*
 * Only the GIC is mapped statically here, as it is accessed before the
 * driver model is up. All other peripherals are mapped on demand by their
 * drivers through the DEVICE_MMIO API.
 */
static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("GICD",
		DT_REG_ADDR_BY_IDX(DT_NODELABEL(gic), 0),
		DT_REG_SIZE_BY_IDX(DT_NODELABEL(gic), 0),
		MT_DEVICE | MPERM_R | MPERM_W),

	MMU_REGION_FLAT_ENTRY("GICC",
		DT_REG_ADDR_BY_IDX(DT_NODELABEL(gic), 1),
		DT_REG_SIZE_BY_IDX(DT_NODELABEL(gic), 1),
		MT_DEVICE | MPERM_R | MPERM_W),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
