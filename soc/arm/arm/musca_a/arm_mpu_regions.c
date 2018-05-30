/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <arch/arm/cortex_m/mpu/arm_mpu.h>

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	{ CONFIG_FLASH_BASE_ADDRESS,
	  "FLASH_0",
	   REGION_FLASH_ATTR(CONFIG_FLASH_BASE_ADDRESS,
			     CONFIG_FLASH_SIZE*1024)},
	/* Region 1 */
	{ CONFIG_SRAM_BASE_ADDRESS,
	  "SRAM_0",
	  REGION_RAM_ATTR(CONFIG_SRAM_BASE_ADDRESS,
			  CONFIG_SRAM_SIZE*1024)}
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
