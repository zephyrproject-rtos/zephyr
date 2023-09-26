/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

#if !defined(CONFIG_XIP)
extern char _rom_attr[];
#endif

static struct arm_mpu_region mpu_regions[] = {

	/* Keep before CODE region so it can be overlapped by SRAM CODE in non-XIP systems */
	{
		.name = "SRAM",
		.base = CONFIG_SRAM_BASE_ADDRESS,
		.attr = REGION_RAM_ATTR(REGION_SRAM_SIZE),
	},

#ifdef CONFIG_XIP
	{
		.name = "FLASH",
		.base = CONFIG_FLASH_BASE_ADDRESS,
		.attr = REGION_FLASH_ATTR(REGION_FLASH_SIZE),
	},
#else
	/* Run from SRAM */
	{
		.name = "CODE",
		.base = CONFIG_SRAM_BASE_ADDRESS,
		.attr = {(uint32_t)_rom_attr},
	},
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
