/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <arch/arm/cortex_m/mpu/arm_mpu.h>

static struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_256K)),
	/* Region 1 */
	MPU_REGION_ENTRY("RAM_0",
			 CONFIG_SRAM_BASE_ADDRESS,
			 REGION_RAM_ATTR(REGION_128K)),
	/* Region 2 */
	MPU_REGION_ENTRY("APB_0",
			 _BEETLE_APB_BASE,
			 REGION_IO_ATTR(REGION_64K)),
	/* Region 3 */
	MPU_REGION_ENTRY("AHB_0",
			 _BEETLE_AHB_BASE,
			 REGION_IO_ATTR(REGION_64K)),
	/* Region 4 */
	MPU_REGION_ENTRY("BITBAND_0",
			 _BEETLE_BITBAND_BASE,
			 REGION_IO_ATTR(REGION_32M)),
	/* Region 5 */
	MPU_REGION_ENTRY("PPB_0",
			 _BEETLE_PPB_BASE,
			 REGION_PPB_ATTR(REGION_1M)),
};

struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
