/* Copyright 2024 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

#define BOOTSTRAP_RAM_BASE_ADDRESS DT_REG_ADDR(DT_NODELABEL(sram_bootstrap))
#define BOOTSTRAP_RAM_SIZE         DT_REG_SIZE(DT_NODELABEL(sram_bootstrap))

#define REGION_BOOTSTRAP_RAM_ATTR(base, size)                                                 \
	{                                                                                       \
		.rbar = FULL_ACCESS_Msk | NON_SHAREABLE_Msk,                                     \
		.mair_idx = MPU_MAIR_INDEX_SRAM,                                                 \
		.r_limit = REGION_LIMIT_ADDR(base, size),                                        \
	}

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH", CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(CONFIG_FLASH_BASE_ADDRESS, CONFIG_FLASH_SIZE * 1024)),

	MPU_REGION_ENTRY("SRAM", CONFIG_SRAM_BASE_ADDRESS,
			 REGION_RAM_ATTR(CONFIG_SRAM_BASE_ADDRESS, CONFIG_SRAM_SIZE * 1024)),

	MPU_REGION_ENTRY("BOOTSTRAP_RAM", BOOTSTRAP_RAM_BASE_ADDRESS,
			 REGION_BOOTSTRAP_RAM_ATTR(BOOTSTRAP_RAM_BASE_ADDRESS, BOOTSTRAP_RAM_SIZE)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
