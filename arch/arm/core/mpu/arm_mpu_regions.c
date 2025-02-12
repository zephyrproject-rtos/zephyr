/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/slist.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
			 REGION_FLASH_ATTR(CONFIG_FLASH_BASE_ADDRESS, \
				 CONFIG_FLASH_SIZE * 1024)),
#else
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
#endif
	/* Region 1 */
	MPU_REGION_ENTRY("SRAM_0",
			 CONFIG_SRAM_BASE_ADDRESS,
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
			 REGION_RAM_ATTR(CONFIG_SRAM_BASE_ADDRESS, \
				 CONFIG_SRAM_SIZE * 1024)),
#else
			 REGION_RAM_ATTR(REGION_SRAM_SIZE)),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
