/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/devicetree.h>

#define SIZE_K(x) ((x) * 1024)
#define SIZE_M(x) (SIZE_K(x) * 1024)

const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH", 0x00000000, REGION_FLASH_ATTR(0x00000000, SIZE_K(512))),

	MPU_REGION_ENTRY("SRAM0_1_2", 0x20000000, REGION_RAM_ATTR(0x20000000, SIZE_K(96))),

	MPU_REGION_ENTRY("SRAM3_CAHB", 0x00C18000, REGION_RAM_ATTR(0x00C18000, SIZE_K(32))),

	MPU_REGION_ENTRY("SRAM3_SAHB", 0x20018000, REGION_RAM_ATTR(0x20018000, SIZE_K(32)))};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
