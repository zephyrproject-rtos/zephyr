/*
 * Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/slist.h>
#include <linker/linker-defs.h>
#include <arch/arm64/cortex_r/mpu/arm_mpu.h>


#define DEVICE_REGION_START 0x80000000UL
#define DEVICE_REGION_END   0xFFFFFFFFUL

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 CONFIG_FLASH_BASE_ADDRESS +
			 KB(CONFIG_FLASH_SIZE),
			 REGION_FLASH_ATTR),

	/* Region 1 zephyr text */
	MPU_REGION_ENTRY("SRAM_0",
			 (uintptr_t)_image_text_start,
			 (uintptr_t)_image_text_end,
			 REGION_RAM_TEXT_ATTR),

	/* Region 2 zephyr rodata */
	MPU_REGION_ENTRY("SRAM_1",
			 (uintptr_t)_image_rodata_start,
			 (uintptr_t)_image_rodata_end,
			 REGION_RAM_RO_ATTR),

	/* Region 3 zephyr data */
	MPU_REGION_ENTRY("SRAM_2",
			 (uintptr_t)__kernel_ram_start,
			 (uintptr_t)__kernel_ram_end,
			 REGION_RAM_ATTR),

	/* Region 4 device region */
	MPU_REGION_ENTRY("DEVICE",
			 DEVICE_REGION_START,
			 DEVICE_REGION_END,
			 REGION_DEVICE_ATTR)
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
