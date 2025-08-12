/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

#define REGION_SRAM1_SHM_BASE_ADDRESS 0x30200000
#define REGION_SRAM1_SHM_SIZE         0x00200000

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY(
		"SRAM2_SHM", REGION_SRAM1_SHM_BASE_ADDRESS,
		REGION_RAM_NOCACHE_ATTR(REGION_SRAM1_SHM_BASE_ADDRESS, REGION_SRAM1_SHM_SIZE)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
