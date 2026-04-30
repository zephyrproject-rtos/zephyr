/*
 * Copyright 2023 Daniel DeGrasse
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <zephyr/arch/arm/mpu/nxp_mpu.h>

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define RAM_BASE CONFIG_SRAM_BASE_ADDRESS
#define RAM_SIZE KB(CONFIG_SRAM_SIZE)
#else
#define RAM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define RAM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_sram))
#endif

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
			 RAM_BASE-1,
			 REGION_BACKGROUND_ATTR),
	/* Region 2 */
	MPU_REGION_ENTRY("BACKGROUND_1",
			 RAM_BASE + RAM_SIZE,
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
			 RAM_BASE,
			 (RAM_BASE + RAM_SIZE - 1),
			 REGION_RAM_ATTR),
};

const struct nxp_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
	.sram_region = 4,
};
