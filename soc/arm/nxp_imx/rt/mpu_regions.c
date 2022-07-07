/*
 * Copyright (c) 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include "../../common/cortex_m/arm_mpu_mem_cfg.h"
#define IS_CHOSEN_SRAM(x) (DT_DEP_ORD(DT_NODELABEL(x)) == DT_DEP_ORD(DT_CHOSEN(zephyr_sram)))

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
#if IS_CHOSEN_SRAM(ocram)
	/* Mark SRAM as noncacheable */
	/* Region 1 */
	MPU_REGION_ENTRY("SRAM_0",
			 CONFIG_SRAM_BASE_ADDRESS,
			 REGION_RAM_NOCACHE_ATTR(REGION_SRAM_SIZE)),

#else
	/* Region 1 */
	MPU_REGION_ENTRY("SRAM_0",
			 CONFIG_SRAM_BASE_ADDRESS,
			 REGION_RAM_ATTR(REGION_SRAM_SIZE)),
	/* Region 2 - OCRAM. Noncacheable. */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(ocram), okay)
	MPU_REGION_ENTRY("OCRAM",
					DT_REG_ADDR(DT_NODELABEL(ocram)),
					REGION_RAM_NOCACHE_ATTR(REGION_256K)),
#endif
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
