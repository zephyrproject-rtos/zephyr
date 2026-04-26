/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

/* userspace threads require RO access to .text and .rodata */
#ifdef CONFIG_USERSPACE
#define PERM_Msk P_RW_U_RO_Msk
#else
#define PERM_Msk P_RW_U_NA_Msk
#endif

static const struct arm_mpu_region mpu_regions[] = {
#if defined CONFIG_SOC_AM2434_R5F0_0
	MPU_REGION_ENTRY("Device", 0x0, REGION_2G, {MPU_RASR_S_Msk | NOT_EXEC | PERM_Msk}),
#endif

	MPU_REGION_ENTRY(
		"SRAM", CONFIG_SRAM_BASE_ADDRESS, REGION_SRAM_SIZE,
		{NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE | PERM_Msk}),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
