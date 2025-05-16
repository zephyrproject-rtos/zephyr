/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/slist.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

static const struct arm_mpu_region mpu_regions[] = {

#if defined(CONFIG_CPU_CORTEX_M7) && defined(CONFIG_CPU_HAS_ARM_MPU) && \
	defined(CONFIG_CPU_HAS_DCACHE)
	/* Erratum 1013783-B (SDEN-1068427): use first region to prevent speculative access
	 * in entire memory space
	 */
	MPU_REGION_ENTRY("BACKGROUND",
			 0,
			 {REGION_4G | MPU_RASR_XN_Msk | P_NA_U_NA_Msk}),
#endif

#ifdef CONFIG_XIP
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
			 REGION_FLASH_ATTR(CONFIG_FLASH_BASE_ADDRESS, \
				 CONFIG_FLASH_SIZE * 1024)),
#else
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
#endif
#endif

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
