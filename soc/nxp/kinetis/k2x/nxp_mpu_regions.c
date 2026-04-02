/*
 * Copyright 2023 Daniel DeGrasse
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <zephyr/arch/arm/mpu/nxp_mpu.h>

static const struct nxp_mpu_region mpu_regions[] = {
	/* Region 0 */
	/* Debugger access can't be disabled; ENET and USB devices will not be able
	 * to access RAM when their regions are dynamically disabled in NXP MPU.
	 */
	MPU_REGION_ENTRY("DEBUGGER_0",
			 0,
			 0xFFFFFFFF,
			 REGION_DEBUGGER_AND_DEVICE_ATTR),

	/* The NXP MPU does not give precedence to memory regions like the ARM
	 * MPU, which means that if one region grants access then another
	 * region cannot revoke access. When hardware stack protection is
	 * enabled, we need to disable supervisor writes to the stack guard
	 * region. Because of the OR semantics we cannot use a single background
	 * region covering all of memory in that case, and instead define two
	 * background regions that together cover the entire address space
	 * except for SRAM, leaving SRAM to be covered only by RAM_U_0 so the
	 * stack guard can deny supervisor writes there.
	 *
	 * When stack guard is disabled no such split is required, so a single
	 * background region covering the full address space is used instead,
	 * reducing boot MPU region consumption from 5 to 4.
	 */
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
	/* Region 3 (stack guard on) / Region 2 (stack guard off) */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 (CONFIG_FLASH_BASE_ADDRESS +
				(CONFIG_FLASH_SIZE * 1024) - 1),
			 REGION_FLASH_ATTR),
	/* Region 4 (stack guard on) / Region 3 (stack guard off) */
	MPU_REGION_ENTRY("RAM_U_0",
			 CONFIG_SRAM_BASE_ADDRESS,
			 (CONFIG_SRAM_BASE_ADDRESS +
				 (CONFIG_SRAM_SIZE * 1024) - 1),
			 REGION_RAM_ATTR),
};

const struct nxp_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
#if defined(CONFIG_MPU_STACK_GUARD)
	.sram_region = 4,
#else
	.sram_region = 3,
#endif
};
