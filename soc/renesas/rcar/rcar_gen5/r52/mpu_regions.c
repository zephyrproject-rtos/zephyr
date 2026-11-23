/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

#define DEVICE_REGION_START (0x18800000u)
#define DEVICE_REGION_LIMIT (0xfbffffffu)

#define REGION_EXECUTABLE_RAM_ATTR(limit)                                      \
	{                                                                      \
		.rbar = P_RW_U_NA_Msk | NON_SHAREABLE_Msk, /* AP, SH */        \
		.mair_idx = MPU_MAIR_INDEX_SRAM,           /* Cacheable */     \
		.r_limit = limit - 1                       /* Region Limit */  \
	}

static const struct arm_mpu_region mpu_regions[] = {
	/* Peripherals */
	MPU_REGION_ENTRY("DEVICE0", DEVICE_REGION_START,
			 REGION_DEVICE_ATTR(DEVICE_REGION_LIMIT)),
	/* SRAM */
	MPU_REGION_ENTRY("SRAM0", DT_CHOSEN_SRAM_ADDR,
			 REGION_EXECUTABLE_RAM_ATTR(DT_CHOSEN_SRAM_ADDR + DT_CHOSEN_SRAM_SIZE))
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
