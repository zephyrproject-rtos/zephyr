/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/arm_mmu.h>
#include <zephyr/devicetree.h>

static const struct arm_mmu_region mmu_regions[] = {
	/*
	 * GPIO is mapped here because pinctrl_renesas_rz.c accesses the registers
	 * directly through the Renesas HAL (R_IOPORT_PinCfg()) without MMIO device API.
	 */
	MMU_REGION_FLAT_ENTRY("GPIO", DT_REG_ADDR_BY_IDX(DT_INST(0, renesas_rz_gpio_common), 0),
			      DT_REG_SIZE_BY_IDX(DT_INST(0, renesas_rz_gpio_common), 0),
			      MT_DEVICE_nGnRnE | MT_RW | MT_DEFAULT_SECURE_STATE),

	/*
	 * SYC and CPG are mapped here (rather than via devicetree) because
	 * soc_early_init_hook() calls the Renesas HAL clock init (bsp_clock_init())
	 * before normal drivers.
	 */
	MMU_REGION_FLAT_ENTRY("SYC", 0x11000000, 0x2000,
			      MT_DEVICE_nGnRnE | MT_RW | MT_DEFAULT_SECURE_STATE),

	MMU_REGION_FLAT_ENTRY("CPG", 0x11010000, 0x1000,
			      MT_DEVICE_nGnRnE | MT_RW | MT_DEFAULT_SECURE_STATE),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
