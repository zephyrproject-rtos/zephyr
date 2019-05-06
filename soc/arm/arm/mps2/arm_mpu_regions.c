/*
 * Copyright (c) 2017-2019 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <generated_dts_board.h>

#include <soc.h>
#include <arch/arm/cortex_m/mpu/arm_mpu.h>

#if defined(CONFIG_ARMV8_M_BASELINE) || \
    defined(CONFIG_ARMV8_M_MAINLINE)
#define MPS2_FLASH_ATTR REGION_FLASH_ATTR(CONFIG_FLASH_BASE_ADDRESS, CONFIG_FLASH_SIZE*1024)
#define MPS2_RAM_ATTR REGION_RAM_ATTR(CONFIG_SRAM_BASE_ADDRESS, CONFIG_SRAM_SIZE*1024)
#else
#define MPS2_FLASH_ATTR REGION_FLASH_ATTR(REGION_4M)
#define MPS2_RAM_ATTR REGION_RAM_ATTR(REGION_2M)
#endif

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 MPS2_FLASH_ATTR),

	MPU_REGION_ENTRY("SRAM_0",
			 CONFIG_SRAM_BASE_ADDRESS,
			 MPS2_RAM_ATTR)
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
