/*
 * Copyright 2023, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

#if !defined(CONFIG_XIP)
extern char _rom_attr[];
#endif

#define REGION_PERIPHERAL_BASE_ADDRESS 0x40000000
#define REGION_PERIPHERAL_SIZE         REGION_512M
#define REGION_PPB_BASE_ADDRESS        0xE0000000
#define REGION_PPB_SIZE                REGION_1M

static struct arm_mpu_region mpu_regions[] = {

	/* ERR011573: use first region to prevent speculative access in entire memory space */
	{
		.name = "UNMAPPED",
		.base = 0,
		.attr = {REGION_4G | MPU_RASR_XN_Msk | P_NA_U_NA_Msk},
	},

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

	{
		.name = "PERIPHERALS",
		.base = REGION_PERIPHERAL_BASE_ADDRESS,
		.attr = REGION_IO_ATTR(REGION_PERIPHERAL_SIZE),
	},

	{
		.name = "PPB",
		.base = REGION_PPB_BASE_ADDRESS,
		.attr = REGION_PPB_ATTR(REGION_PPB_SIZE),
	},
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
