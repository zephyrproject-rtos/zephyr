/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * i.MXRT118x (CM7) selects CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS.
 * Some boards provide their own mpu_config via board sources, but for
 * build-only virtual boards we still need a definition.
 *
 * Keep this minimal: define basic Flash + SRAM regions using generic MPU
 * helpers.
 */

#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define RAM_BASE CONFIG_SRAM_BASE_ADDRESS
#else
#define RAM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#endif

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0: Flash */
	MPU_REGION_ENTRY("FLASH_0", CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),

	/* Region 1: SRAM */
	MPU_REGION_ENTRY("SRAM_0", RAM_BASE, REGION_RAM_ATTR(REGION_SRAM_SIZE)),
};

__weak const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
