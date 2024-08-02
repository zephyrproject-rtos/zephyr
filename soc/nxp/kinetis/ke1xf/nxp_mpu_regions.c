/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <zephyr/arch/arm/mpu/nxp_mpu.h>

static const struct nxp_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("DEBUGGER_0",
			 0,
			 0xFFFFFFFF,
			 REGION_DEBUG_ATTR),

	/* The NXP MPU does not give precedence to memory regions like the ARM
	 * MPU, which means that if one region grants access then another
	 * region cannot revoke access. If an application enables hardware
	 * stack protection, we need to disable supervisor writes from the core
	 * to the stack guard region. As a result, we cannot have a single
	 * background region that enables supervisor read/write access from the
	 * core to the entire address space, and instead define two background
	 * regions that together cover the entire address space except for
	 * SRAM.
	 */

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
	/* Region 3 */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 (CONFIG_FLASH_BASE_ADDRESS +
				(CONFIG_FLASH_SIZE * 1024) - 1),
			 REGION_FLASH_ATTR),
	/* Region 4 */
	MPU_REGION_ENTRY("RAM_U_0",
			 CONFIG_SRAM_BASE_ADDRESS,
			 (CONFIG_SRAM_BASE_ADDRESS +
				(CONFIG_SRAM_SIZE * 1024) - 1),
			 REGION_RAM_ATTR),
};

const struct nxp_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
	.sram_region = 4,
};
