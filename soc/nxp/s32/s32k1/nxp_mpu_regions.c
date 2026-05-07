/*
 * Copyright 2023 NXP
 *
 * Based on soc/soc_legacy/arm/nxp_kinetis/ke1xf/nxp_mpu_regions.c, which is:
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/arch/arm/mpu/nxp_mpu.h>

static const struct nxp_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("DEBUGGER",
			 0,
			 0xFFFFFFFF,
			 REGION_DEBUGGER_AND_DEVICE_ATTR),

#if defined(CONFIG_MPU_STACK_GUARD)
	/* Region 1 */
	MPU_REGION_ENTRY("BACKGROUND_0",
			 0,
			 CONFIG_SRAM_BASE_ADDRESS-1,
			 REGION_BACKGROUND_ATTR),
	/* Region 2 */
	MPU_REGION_ENTRY("BACKGROUND_1",
			 CONFIG_SRAM_BASE_ADDRESS +
				 (CONFIG_SRAM_SIZE * 1024),
			 0xFFFFFFFF,
			 REGION_BACKGROUND_ATTR),
#else
	/* Region 1: single background region covering full address space.
	 * Safe when MPU_STACK_GUARD is disabled because no region needs to
	 * deny supervisor writes to a sub-region of SRAM.
	 */
	MPU_REGION_ENTRY("BACKGROUND_0",
			 0,
			 0xFFFFFFFF,
			 REGION_BACKGROUND_ATTR),
#endif /* CONFIG_MPU_STACK_GUARD */

#if defined(CONFIG_XIP)
	/* Region 3 */
	MPU_REGION_ENTRY("SRAM",
			 CONFIG_SRAM_BASE_ADDRESS,
			 (CONFIG_SRAM_BASE_ADDRESS +
				 (CONFIG_SRAM_SIZE * 1024) - 1),
			 REGION_RAM_ATTR),

	/* Region 4 */
	MPU_REGION_ENTRY("FLASH",
			 CONFIG_FLASH_BASE_ADDRESS,
			 (CONFIG_FLASH_BASE_ADDRESS +
				(CONFIG_FLASH_SIZE * 1024) - 1),
			 REGION_FLASH_ATTR),
#else
	/* Region 3 */
	MPU_REGION_ENTRY("SRAM",
			 CONFIG_SRAM_BASE_ADDRESS,
			 (CONFIG_SRAM_BASE_ADDRESS +
				 (CONFIG_SRAM_SIZE * 1024) - 1),
			 REGION_FLASH_ATTR),
#endif
};

const struct nxp_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
#if defined(CONFIG_MPU_STACK_GUARD)
	.sram_region = 3,
#else
	.sram_region = 2,
#endif
};
