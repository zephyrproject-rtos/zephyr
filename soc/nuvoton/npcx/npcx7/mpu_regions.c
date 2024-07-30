/*
 * Copyright (c) 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH_0_0",
			 CONFIG_FLASH_BASE_ADDRESS & -KB(256),
			 REGION_FLASH_ATTR(REGION_256K)),
#if CONFIG_FLASH_SIZE > 256
	MPU_REGION_ENTRY("FLASH_0_1",
			 (CONFIG_FLASH_BASE_ADDRESS + KB(256)) & -KB(256),
			 REGION_FLASH_ATTR(REGION_256K)),
#endif
	MPU_REGION_ENTRY("SRAM_0",
			 CONFIG_SRAM_BASE_ADDRESS,
			 REGION_RAM_ATTR(REGION_SRAM_SIZE)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
