/* Copyright 2024 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

#define BOOTSTRAP_SAHB_RAM_BASE_ADDRESS DT_REG_ADDR(DT_NODELABEL(sram_bootstrap_sahb))
#define BOOTSTRAP_CBUS_RAM_BASE_ADDRESS DT_REG_ADDR(DT_NODELABEL(sram_bootstrap_cbus))
#define BOOTSTRAP_RAM_SIZE              DT_REG_SIZE(DT_NODELABEL(sram_bootstrap))

#define REGION_BOOTSTRAP_RAM_ATTR(base, size)                                                 \
	{                                                                                       \
		.rbar = FULL_ACCESS_Msk | NON_SHAREABLE_Msk,                                     \
		.mair_idx = MPU_MAIR_INDEX_SRAM,                                                 \
		.r_limit = REGION_LIMIT_ADDR(base, size),                                        \
	}

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define RAM_BASE CONFIG_SRAM_BASE_ADDRESS
#define RAM_SIZE (CONFIG_SRAM_SIZE * 1024UL)
#else
#define RAM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define RAM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_sram))
#endif

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH", CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(CONFIG_FLASH_BASE_ADDRESS, CONFIG_FLASH_SIZE * 1024)),

	MPU_REGION_ENTRY("SRAM", RAM_BASE,
			 REGION_RAM_ATTR(RAM_BASE, RAM_SIZE)),

	MPU_REGION_ENTRY(
		"BOOTSTRAP_SAHB_RAM", BOOTSTRAP_SAHB_RAM_BASE_ADDRESS,
		REGION_BOOTSTRAP_RAM_ATTR(BOOTSTRAP_SAHB_RAM_BASE_ADDRESS, BOOTSTRAP_RAM_SIZE)),

	MPU_REGION_ENTRY(
		"BOOTSTRAP_CBUS_RAM", BOOTSTRAP_CBUS_RAM_BASE_ADDRESS,
		REGION_BOOTSTRAP_RAM_ATTR(BOOTSTRAP_CBUS_RAM_BASE_ADDRESS, BOOTSTRAP_RAM_SIZE)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
