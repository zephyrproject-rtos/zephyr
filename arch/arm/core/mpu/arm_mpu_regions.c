/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>
#include <zephyr/devicetree.h>

#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#define FLASH_0_NODE DT_CHOSEN(zephyr_code_partition)
#define FLASH_0_BASE DT_REG_ADDR(FLASH_0_NODE)
#define FLASH_0_SIZE DT_REG_SIZE(FLASH_0_NODE)
#else
#define FLASH_0_BASE CONFIG_FLASH_BASE_ADDRESS
#define FLASH_0_SIZE (CONFIG_FLASH_SIZE * 1024)
#endif

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 FLASH_0_BASE,
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
			 REGION_FLASH_ATTR(FLASH_0_BASE, FLASH_0_SIZE)),
#else
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
#endif
	/* Region 1 */
	MPU_REGION_ENTRY("SRAM_0",
			 CONFIG_SRAM_BASE_ADDRESS,
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
			 REGION_RAM_ATTR(CONFIG_SRAM_BASE_ADDRESS, \
				 CONFIG_SRAM_SIZE * 1024)),
#else
			 REGION_RAM_ATTR(REGION_SRAM_SIZE)),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
