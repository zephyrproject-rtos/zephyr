/*
 * Copyright (c) 2025 Andy Lin <andylinpersonal@gmail.com>.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hardware/regs/addressmap.h>

#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

#define RP2040_BOOTROM_REGION_SIZE REGION_16K

static const struct arm_mpu_region mpu_regions[] = {
#if IS_ENABLED(CONFIG_RPI_PICO_USE_ROMFUNC)
	MPU_REGION_ENTRY("BOOTROM", ROM_BASE, REGION_FLASH_ATTR(RP2040_BOOTROM_REGION_SIZE)),
#endif

#if IS_ENABLED(CONFIG_XIP)
	MPU_REGION_ENTRY("FLASH", CONFIG_FLASH_BASE_ADDRESS, REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
#endif

	MPU_REGION_ENTRY("SRAM_0", CONFIG_SRAM_BASE_ADDRESS, REGION_RAM_ATTR(REGION_SRAM_SIZE)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
