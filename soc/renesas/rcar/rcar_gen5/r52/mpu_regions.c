/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

#define DEVICE_0_REGION_START 0x18800000U
#define DEVICE_0_REGION_END   0x38000000U
#define DEVICE_1_REGION_START 0xC0000000U
#define DEVICE_1_REGION_END   0xFFFFFFC0U

#define REGION_EXECUTABLE_RAM_ATTR(limit)                                                          \
	{                                                                                          \
		.rbar = P_RW_U_NA_Msk | NON_SHAREABLE_Msk, /* AP, SH */                            \
		.mair_idx = MPU_MAIR_INDEX_SRAM,           /* Cacheable */                         \
		.r_limit = limit - 1                       /* Region Limit */                      \
	}

static const struct arm_mpu_region mpu_regions[] = {
	/* SRAM */
	MPU_REGION_ENTRY("SRAM0", DT_CHOSEN_SRAM_ADDR,
			 REGION_EXECUTABLE_RAM_ATTR(DT_CHOSEN_SRAM_ADDR + DT_CHOSEN_SRAM_SIZE)),

	MPU_REGION_ENTRY("DEVICE_0", DEVICE_0_REGION_START,
			 REGION_DEVICE_ATTR(DEVICE_0_REGION_END)),

	MPU_REGION_ENTRY("DEVICE_1", DEVICE_1_REGION_START,
			 REGION_DEVICE_ATTR(DEVICE_1_REGION_END)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
