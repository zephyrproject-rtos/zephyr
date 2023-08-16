/*
 * Copyright (c) 2021-2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/arch/arm64/cortex_r/arm_mpu.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 CONFIG_FLASH_BASE_ADDRESS +
			 KB(CONFIG_FLASH_SIZE),
			 REGION_FLASH_ATTR),

	/* Region 1 zephyr text */
	MPU_REGION_ENTRY("SRAM_0",
			 (uintptr_t)__text_region_start,
			 (uintptr_t)__text_region_end,
			 REGION_RAM_TEXT_ATTR),

	/* Region 2 zephyr rodata */
	MPU_REGION_ENTRY("SRAM_1",
			 (uintptr_t)__rodata_region_start,
			 (uintptr_t)__rodata_region_end,
			 REGION_RAM_RO_ATTR),

	/* Region 3 zephyr data */
	MPU_REGION_ENTRY("SRAM_2",
#ifdef CONFIG_USERSPACE
			 (uintptr_t)_app_smem_start,
#else
			 (uintptr_t)__kernel_ram_start,
#endif
			 (uintptr_t)__kernel_ram_end,
			 REGION_RAM_ATTR),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
