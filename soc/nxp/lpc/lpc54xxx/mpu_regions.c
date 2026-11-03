/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

#define REGION_PPB_BASE_ADDRESS 0xE0000000U
#define REGION_PPB_SIZE REGION_1M

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH", CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
	MPU_REGION_ENTRY("SRAM", DT_CHOSEN_SRAM_ADDR,
			 REGION_RAM_ATTR(REGION_SRAM_SIZE)),
	MPU_REGION_ENTRY("PPB", REGION_PPB_BASE_ADDRESS,
			 REGION_IO_ATTR(REGION_PPB_SIZE)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
