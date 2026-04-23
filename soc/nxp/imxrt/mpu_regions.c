/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SDRAM_BASE_ADDR 0x80000000

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define RAM_BASE CONFIG_SRAM_BASE_ADDRESS
#else
#define RAM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#endif

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
	/* Region 1 */
	MPU_REGION_ENTRY("SRAM_0",
			 RAM_BASE,
			 REGION_RAM_ATTR(REGION_SRAM_SIZE)),

#ifndef CONFIG_NXP_IMX_EXTERNAL_SDRAM
	/*
	 * Region 2 - mark SDRAM0 as device type memory to prevent core
	 * from executing speculative prefetches against this region when
	 * no SDRAM is present.
	 */
	MPU_REGION_ENTRY("SDRAM0", SDRAM_BASE_ADDR, REGION_IO_ATTR(REGION_512M)),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
