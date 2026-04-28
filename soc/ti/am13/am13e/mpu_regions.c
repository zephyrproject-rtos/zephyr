/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/devicetree.h>

#if IS_ENABLED(CONFIG_XIP)
#define FLASH_NODE DT_NODELABEL(flash0)
#endif

const struct arm_mpu_region mpu_regions[] = {
#if IS_ENABLED(CONFIG_XIP)
	MPU_REGION_ENTRY("FLASH", DT_REG_ADDR(FLASH_NODE),
			 REGION_FLASH_ATTR(DT_REG_ADDR(FLASH_NODE), DT_REG_SIZE(FLASH_NODE))),
#endif

	MPU_REGION_ENTRY("SRAM0_1_2SAHB", DT_REG_ADDR(DT_NODELABEL(sram0_1_2)),
			 REGION_RAM_ATTR(DT_REG_ADDR(DT_NODELABEL(sram0_1_2)),
					 DT_REG_SIZE(DT_NODELABEL(sram0_1_2)))),
	MPU_REGION_ENTRY("SRAM3_CAHB", DT_REG_ADDR(DT_NODELABEL(sram3_cahb)),
			 REGION_RAM_ATTR(DT_REG_ADDR(DT_NODELABEL(sram3_cahb)),
					 DT_REG_ADDR(DT_NODELABEL(sram3_cahb)))),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
