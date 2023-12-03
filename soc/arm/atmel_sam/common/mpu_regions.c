/*
 * Copyright (c) 2023 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

#define REGION_ITCM_BASE_ADDRESS			0x00000000U
#define REGION_DTCM_BASE_ADDRESS			0x20000000U
#define REGION_QSPI_BASE_ADDRESS			0x80000000U

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH", CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),

	MPU_REGION_ENTRY("RAM_NOCACHE", CONFIG_SRAM_BASE_ADDRESS,
			 REGION_RAM_NOCACHE_ATTR(REGION_8K)),

	MPU_REGION_ENTRY("RAM", CONFIG_SRAM_BASE_ADDRESS,
			 REGION_RAM_ATTR(REGION_SRAM_SIZE)),

	/*
	 * ITCM[0x0000_0000 - 0x0000_FFFF]:
	 * Memory with Normal type, not shareable, non-cacheable
	 */
	MPU_REGION_ENTRY("ITCM", REGION_ITCM_BASE_ADDRESS,
			 { ARM_MPU_RASR(0, ARM_MPU_AP_FULL,
			   1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_32KB) }),

	/*
	 * DTCM[0x2000_0000 - 0x2000_FFFF]:
	 * Memory with Normal type, not shareable, non-cacheable
	 */
	MPU_REGION_ENTRY("DTCM", REGION_DTCM_BASE_ADDRESS,
			 { ARM_MPU_RASR(0, ARM_MPU_AP_FULL,
			   1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_32KB) }),

	/*
	 * QSPI[0x8000_0000 - 0x9FFF_FFFF]:
	 * Memory with Normal type, not shareable, cacheable
	 */
	MPU_REGION_ENTRY("QSPI", REGION_QSPI_BASE_ADDRESS,
			 { ARM_MPU_RASR(0, ARM_MPU_AP_FULL,
			   1, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_512MB) }),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
