/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <arch/arm/cortex_m/mpu/nxp_mpu.h>

#define FLEXBUS_BASE_ADDRESS 0x08000000
#define SRAM_L_BASE_ADDRESS 0x1FFF0000
#define DEVICE_S_BASE_ADDRESS 0x20030000

static struct nxp_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("DEBUGGER_0",
			 0,
			 0xFFFFFFFF,
			 0),
	/* Region 1 */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 0x07FFFFFF,
			 REGION_FLASH_ATTR),
	/* Region 2 */
	/*
	 * This region (Flexbus + FlexNVM) is bigger than the FLEXBUS one in
	 * order to save 1 region allocation in the MPU.
	 */
	MPU_REGION_ENTRY("FLEXBUS_0",
			 FLEXBUS_BASE_ADDRESS,
			 0x1BFFFFFF,
			 REGION_IO_ATTR),
	/* Region 3 */
	MPU_REGION_ENTRY("RAM_L_0",
			 SRAM_L_BASE_ADDRESS,
			 0x1FFFFFFF,
			 REGION_RAM_ATTR),
	/* Region 4 */
	MPU_REGION_ENTRY("RAM_U_0",
			 CONFIG_SRAM_BASE_ADDRESS,
			 (CONFIG_SRAM_BASE_ADDRESS +
				 (CONFIG_SRAM_SIZE * 1024) - 1),
			 REGION_RAM_ATTR),
	/* Region 5 */
	MPU_REGION_ENTRY("DEVICE_0",
			 DEVICE_S_BASE_ADDRESS,
			 0xFFFFFFFF,
			 REGION_IO_ATTR),
};

struct nxp_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
	.sram_region = 4,
};
