/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/linker/linker-defs.h>

#define DEVICE_0_REGION_START 0x18800000U
#define DEVICE_0_REGION_END   0x38000000U
#define DEVICE_1_REGION_START 0xC0000000U
#define DEVICE_1_REGION_END   0xFFFFFFC0U

#define DRAM_START 0x40000000U
#define DRAM_END   0xC0000000U
#define SRAM_START DT_CHOSEN_SRAM_ADDR
#define SRAM_END   (DT_CHOSEN_SRAM_ADDR + DT_CHOSEN_SRAM_SIZE)

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("SRAM_TEXT", (uintptr_t)__rom_region_start,
			 REGION_RAM_TEXT_ATTR((uintptr_t)__rodata_region_start)),

	MPU_REGION_ENTRY("SRAM_RODATA", (uintptr_t)__rodata_region_start,
			 REGION_RAM_RO_ATTR((uintptr_t)__rodata_region_end)),

	MPU_REGION_ENTRY("SRAM_DATA", (uintptr_t)__rom_region_end,
			 REGION_RAM_ATTR((uintptr_t)__kernel_ram_end)),

#if (SRAM_START >= DRAM_START) && (SRAM_START < DRAM_END)
#if SRAM_START > DRAM_START
	MPU_REGION_ENTRY("DRAM_0", DRAM_START, REGION_RAM_NOCACHE_ATTR(SRAM_START)),
#endif
#if SRAM_END < DRAM_END
	MPU_REGION_ENTRY("DRAM_1", SRAM_END, REGION_RAM_NOCACHE_ATTR(DRAM_END)),
#endif
#else
	MPU_REGION_ENTRY("DRAM", DRAM_START, REGION_RAM_NOCACHE_ATTR(DRAM_END)),
#endif

	MPU_REGION_ENTRY("DEVICE_0", DEVICE_0_REGION_START,
			 REGION_DEVICE_ATTR(DEVICE_0_REGION_END)),

	MPU_REGION_ENTRY("DEVICE_1", DEVICE_1_REGION_START,
			 REGION_DEVICE_ATTR(DEVICE_1_REGION_END)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
