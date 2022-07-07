/*
 * Copyright (c) 2020 Mario Jaun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/devicetree_regions.h>
#include "../../common/cortex_m/arm_mpu_mem_cfg.h"

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH", CONFIG_FLASH_BASE_ADDRESS,
					 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
	MPU_REGION_ENTRY("SRAM", CONFIG_SRAM_BASE_ADDRESS,
					 REGION_RAM_ATTR(REGION_SRAM_SIZE)),
#if DT_NODE_HAS_STATUS(DT_NODELABEL(sram3), okay) && \
		DT_NODE_HAS_STATUS(DT_NODELABEL(mac), okay)
	MPU_REGION_ENTRY("SRAM3_ETH_BUF",
					 DT_REG_ADDR(DT_NODELABEL(sram3)),
					 REGION_RAM_NOCACHE_ATTR(REGION_16K)),
	MPU_REGION_ENTRY("SRAM3_ETH_DESC",
					 DT_REG_ADDR(DT_NODELABEL(sram3)),
					 REGION_PPB_ATTR(REGION_256B)),
#endif

	/* DT-defined regions */
	LINKER_DT_REGION_MPU(ARM_MPU_REGION_INIT)
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
