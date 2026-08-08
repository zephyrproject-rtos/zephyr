/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mmu/arm_mmu.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

/*
 * The ARMv8-A AArch32 vector table is mapped by the arch-level MMU code
 * (see arch/arm/core/mmu/arm_mmu.c), so only device regions need to be
 * described here.
 */
static const struct arm_mmu_region mmu_regions[] = {
	MMU_REGION_FLAT_ENTRY("GICD",
			      DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic_v3), 0),
			      DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic_v3), 0),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
	MMU_REGION_FLAT_ENTRY("GICR",
			      DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic_v3), 1),
			      DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic_v3), 1),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
	MMU_REGION_FLAT_ENTRY("UART",
			      DT_REG_ADDR(DT_NODELABEL(uart0)),
			      DT_REG_SIZE(DT_NODELABEL(uart0)),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
