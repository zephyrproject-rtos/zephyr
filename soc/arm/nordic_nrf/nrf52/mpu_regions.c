/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/slist.h>
#include <arch/arm/cortex_m/mpu/arm_mpu.h>

#include "mpu_mem_cfg.h"

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
#if defined(REGION_FLASH_1_SIZE)
	MPU_REGION_ENTRY("FLASH_1",
			 REGION_FLASH_1_START,
			 REGION_FLASH_ATTR(REGION_FLASH_1_SIZE)),
#endif /* REGION_FLASH_1_SIZE */
	/* Region 1 */
	MPU_REGION_ENTRY("SRAM_0",
			 CONFIG_SRAM_BASE_ADDRESS,
			 REGION_RAM_ATTR(REGION_SRAM_0_SIZE)),

#if defined(REGION_SRAM_1_SIZE)
	MPU_REGION_ENTRY("SRAM_1",
			 REGION_SRAM_1_START,
			 REGION_RAM_ATTR(REGION_SRAM_1_SIZE)),
#endif /* REGION_SRAM_1_SIZE */
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
