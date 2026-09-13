/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

#define NODE_ATCM DT_NODELABEL(atcm)
#define NODE_BTCM DT_NODELABEL(btcm)

extern const uint32_t __rom_region_start;
extern const uint32_t __rom_region_mpu_size_bits;

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("Device", 0x0, REGION_2G, {MPU_RASR_S_Msk | NOT_EXEC | P_RW_U_NA_Msk}),

#if DT_NODE_EXISTS(NODE_ATCM)
	/* ATCM - Mark as executable since it contains the vector table */
	MPU_REGION_ENTRY(
		"ATCM", DT_REG_ADDR(NODE_ATCM),
		REGION_CUSTOMED_MEMORY_SIZE(DT_REG_SIZE(NODE_ATCM) >> 10),
		{P_RO_U_NA_Msk | NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE}),
#endif

#if DT_NODE_EXISTS(NODE_BTCM)
	/* BTCM */
	MPU_REGION_ENTRY("BTCM", DT_REG_ADDR(NODE_BTCM),
			 REGION_CUSTOMED_MEMORY_SIZE(DT_REG_SIZE(NODE_BTCM) >> 10),
			 {P_RW_U_NA_Msk | NOT_EXEC |
			  NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE}),
#endif

	/* Current SRAM region */
	MPU_REGION_ENTRY("SRAM", DT_CHOSEN_SRAM_ADDR,
			 REGION_CUSTOMED_MEMORY_SIZE(DT_CHOSEN_SRAM_SIZE >> 10),
			 {P_RW_U_NA_Msk | NOT_EXEC |
			  NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE}),

	/* .rodata and .text should be read-only and executable */
	MPU_REGION_ENTRY(
		"ROM region", (uint32_t)&__rom_region_start, (uint32_t)&__rom_region_mpu_size_bits,
		{P_RO_U_RO_Msk | NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE}),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
