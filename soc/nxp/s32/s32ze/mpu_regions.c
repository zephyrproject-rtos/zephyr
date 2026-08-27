/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-defs.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

#define DEVICE_REGION_START 0x40000000UL
#define DEVICE_REGION_END   0x76FFFFFFUL

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("SRAM_TEXT",
			 (uintptr_t)__rom_region_start,
			 REGION_RAM_TEXT_ATTR((uintptr_t)__rodata_region_start)),

	MPU_REGION_ENTRY("SRAM_RODATA",
			 (uintptr_t)__rodata_region_start,
#ifdef CONFIG_XIP
			 REGION_RAM_RO_ATTR(CONFIG_FLASH_BASE_ADDRESS + KB(CONFIG_FLASH_SIZE))
#else
			 REGION_RAM_RO_ATTR((uintptr_t)__rodata_region_end)
#endif
			 ),

	MPU_REGION_ENTRY("SRAM_DATA",
#ifdef CONFIG_XIP
			 (uintptr_t)_image_ram_start,
#else
			 (uintptr_t)__rom_region_end,
#endif
			 REGION_RAM_ATTR((uintptr_t)__kernel_ram_end)),

	MPU_REGION_ENTRY("DEVICE",
			 DEVICE_REGION_START,
			 REGION_DEVICE_ATTR(DEVICE_REGION_END)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
