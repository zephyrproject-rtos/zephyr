/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

extern const uint32_t __rom_region_start;
extern const uint32_t __rom_region_mpu_size_bits;

static const struct arm_mpu_region mpu_regions[] = {

	/* clang-format off */
#if DT_NODE_EXISTS(DT_NODELABEL(itcm))
	/* Vectors is relocated to ITCM region */
	MPU_REGION_ENTRY(
		"itcm",
		DT_REG_ADDR(DT_NODELABEL(itcm)),
		REGION_CUSTOMED_MEMORY_SIZE(DT_REG_SIZE(DT_NODELABEL(itcm)) / 1024),
		{.rasr = P_RO_U_NA_Msk |
			 NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE}),
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(dtcm))
	MPU_REGION_ENTRY(
		"dtcm",
		DT_REG_ADDR(DT_NODELABEL(dtcm)),
		REGION_CUSTOMED_MEMORY_SIZE(DT_REG_SIZE(DT_NODELABEL(dtcm)) / 1024),
		{.rasr = P_RW_U_RW_Msk |
			 NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE |
			 NOT_EXEC}),
#endif

	/* Basic SRAM mapping is all data, R/W + XN */
	MPU_REGION_ENTRY(
		"sram",
		CONFIG_SRAM_BASE_ADDRESS,
		REGION_SRAM_SIZE,
		{.rasr = P_RW_U_RW_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE |
			 NOT_EXEC}),

	/* Add rom_region mapping for SRAM which is RO + executable */
	MPU_REGION_ENTRY(
		"rom_region",
		(uint32_t)(&__rom_region_start),
		(uint32_t)(&__rom_region_mpu_size_bits),
		{.rasr = P_RO_U_RO_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE}),

	MPU_REGION_ENTRY(
		"peripherals",
		0x10000000,
		REGION_256M,
		{.rasr = P_RW_U_NA_Msk |
			 DEVICE_SHAREABLE |
			 NOT_EXEC}),

	MPU_REGION_ENTRY(
		"xspi",
		0x20000000,
		REGION_256M,
		{.rasr = P_RO_U_RO_Msk |
			 (4 << MPU_RASR_TEX_Pos) |
			 MPU_RASR_B_Msk}),

	/* clang-format on */
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
