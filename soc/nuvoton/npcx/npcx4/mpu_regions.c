/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

static const struct arm_mpu_region mpu_regions[] = {
	/* Covers [base rounded down to 256K, +256K), e.g. 0x10040000 - 0x10080000 */
	MPU_REGION_ENTRY("FLASH_0_0",
			 CONFIG_FLASH_BASE_ADDRESS & -KB(256),
			 REGION_FLASH_ATTR(REGION_256K)),
#if CONFIG_FLASH_SIZE > 256
	/* Covers the next 256K, e.g. 0x10080000 - 0x100c0000 */
	MPU_REGION_ENTRY("FLASH_0_1",
			 (CONFIG_FLASH_BASE_ADDRESS + KB(256)) & -KB(256),
			 REGION_FLASH_ATTR(REGION_256K)),
#endif
	/* Covers the SRAM region, e.g. 0x200c0000 - 0x200e0000 */
	MPU_REGION_ENTRY("SRAM_0",
			 DT_CHOSEN_SRAM_ADDR,
			 REGION_RAM_ATTR(REGION_SRAM_SIZE)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
