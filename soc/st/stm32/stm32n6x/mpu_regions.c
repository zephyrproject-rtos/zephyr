/*
 * Copyright The Zephyr Project Contributors
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-defs.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

extern const uint32_t __rom_region_limit[];
extern const uint32_t __image_ram_limit[];

static const struct arm_mpu_region mpu_regions[] = {
	{
		.base = (uint32_t)__rom_region_start,
		.name = "SRAM_RO",
		.attr = {
			.rbar = RO_Msk | NON_SHAREABLE_Msk,
			.mair_idx = MPU_MAIR_INDEX_FLASH,
			.pxn = !PRIV_EXEC_NEVER,
			.r_limit = (uint32_t)__rom_region_limit,
		},
	},
	{
		.base = (uint32_t)_image_ram_start,
		.name = "SRAM_RW",
		.attr = {
			.rbar = NOT_EXEC | P_RW_U_NA_Msk | NON_SHAREABLE_Msk,
			.mair_idx = MPU_MAIR_INDEX_SRAM,
			.pxn = !PRIV_EXEC_NEVER,
			.r_limit = (uint32_t)__image_ram_limit,
		},
	}
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
