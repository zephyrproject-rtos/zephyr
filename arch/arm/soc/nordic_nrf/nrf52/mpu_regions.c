/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/slist.h>
#include <arch/arm/cortex_m/mpu/arm_mpu.h>

#include "mpu_mem_cfg.h"

#define XICR_BASE	0x10000000
#define PERIPH_BASE	0x40000000
#define M4_PPB_BASE	0xE0000000

static struct arm_mpu_region mpu_regions[] = {
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

struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
