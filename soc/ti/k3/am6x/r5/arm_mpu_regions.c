/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

static const struct arm_mpu_region mpu_regions[] = {
#if defined CONFIG_SOC_AM2434_R5F0_0
	MPU_REGION_ENTRY("Device", 0x0, REGION_2G, {MPU_RASR_S_Msk | NOT_EXEC | P_RW_U_RO_Msk}),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
