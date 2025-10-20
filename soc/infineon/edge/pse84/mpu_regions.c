/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

/* We are expected to give *CONFIG_SIZE* in KB, but REGION_ATTR
 * expects bytes, so we multiply by 1024 to convert.
 */
static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH", CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(CONFIG_FLASH_BASE_ADDRESS, CONFIG_FLASH_SIZE * 1024)),

	MPU_REGION_ENTRY("SRAM", CONFIG_SRAM_BASE_ADDRESS,
			 REGION_RAM_ATTR(CONFIG_SRAM_BASE_ADDRESS, CONFIG_SRAM_SIZE * 1024)),

#if DT_NODE_EXISTS(DT_NODELABEL(itcm))
	MPU_REGION_ENTRY(
		"ITCM", DT_REG_ADDR(DT_NODELABEL(itcm)),
		REGION_RAM_ATTR(DT_REG_ADDR(DT_NODELABEL(itcm)), DT_REG_SIZE(DT_NODELABEL(itcm)))),
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(dtcm))
	MPU_REGION_ENTRY(
		"DTCM", DT_REG_ADDR(DT_NODELABEL(dtcm)),
		REGION_RAM_ATTR(DT_REG_ADDR(DT_NODELABEL(dtcm)), DT_REG_SIZE(DT_NODELABEL(dtcm)))),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
