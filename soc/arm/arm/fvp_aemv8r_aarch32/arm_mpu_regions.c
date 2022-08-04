/*
 * Copyright (c) 2022 IoT.bzh
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/slist.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/arch/arm/aarch32/mpu/arm_mpu.h>

#define DEVICE_REGION_START 0x80000000UL
#define DEVICE_REGION_END   0xFFFFFFFFUL

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 zephyr vector */
	MPU_REGION_ENTRY("vector",
			 (uintptr_t)_vector_start,
			 REGION_RAM_TEXT_ATTR((uintptr_t)_vector_end)),

	/* Region 1 zephyr text */
	MPU_REGION_ENTRY("SRAM_0",
			 (uintptr_t)__text_region_start,
			 REGION_RAM_TEXT_ATTR((uintptr_t)__rodata_region_start)),

	/* Region 2 zephyr rodata */
	MPU_REGION_ENTRY("SRAM_1",
			 (uintptr_t)__rodata_region_start,
			 REGION_RAM_RO_ATTR((uintptr_t)__rodata_region_end)),

	/* Region 3 zephyr data */
	MPU_REGION_ENTRY("SRAM_2",
#ifdef CONFIG_USERSPACE
			 (uintptr_t)_app_smem_start,
#else
			 (uintptr_t)__kernel_ram_start,
#endif
			 REGION_RAM_ATTR((uintptr_t)__kernel_ram_end)),

	/* Region 4 device region */
	MPU_REGION_ENTRY("DEVICE",
			 DEVICE_REGION_START,
			 REGION_DEVICE_ATTR(DEVICE_REGION_END)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
