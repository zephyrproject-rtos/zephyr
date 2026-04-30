/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/slist.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

#ifdef CONFIG_ARM_MPU_SRAM_WRITE_THROUGH
#define ARM_MPU_SRAM_REGION_ATTR  REGION_RAM_WT_ATTR
#else
#define ARM_MPU_SRAM_REGION_ATTR  REGION_RAM_ATTR
#endif

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define RAM_BASE CONFIG_SRAM_BASE_ADDRESS
#define RAM_SIZE KB(CONFIG_SRAM_SIZE)
#else
#define RAM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define RAM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_sram))
#endif

static const struct arm_mpu_region mpu_regions[] = {
#ifdef CONFIG_XIP
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
			 REGION_FLASH_ATTR(CONFIG_FLASH_BASE_ADDRESS, \
				 CONFIG_FLASH_SIZE * 1024)),
#else
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
#endif
#endif

	/* Region 1 */
	MPU_REGION_ENTRY("SRAM_0",
			 RAM_BASE,
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
			 ARM_MPU_SRAM_REGION_ATTR(RAM_BASE,
				 RAM_SIZE)),
#else
			 ARM_MPU_SRAM_REGION_ATTR(REGION_SRAM_SIZE)),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
