/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 Lexmark International, Inc.
 * Copyright (c) 2025 Immo Birnbaum
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

extern const uint32_t __rom_region_start;
extern const uint32_t __rom_region_mpu_size_bits;

static const struct arm_mpu_region mpu_regions[] = {
	/*
	 * The address of the vectors is determined by arch/arm/core/cortex_a_r/prep_c.c
	 * -> for v7-R, there's no other option than 0x0, HIVECS always gets cleared
	 */
	MPU_REGION_ENTRY(
		"vectors",
		0x00000000,
		REGION_64B,
		{.rasr = P_RO_U_NA_Msk |
			 NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE}),
	/* Basic SRAM mapping is all data, R/W + XN */
	MPU_REGION_ENTRY(
		"sram",
		CONFIG_SRAM_BASE_ADDRESS,
		REGION_SRAM_SIZE,
		{.rasr = P_RW_U_NA_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE |
			 NOT_EXEC}),
#if defined(CONFIG_XIP)
	/* .text and .rodata (=rom_region) are in flash, must be RO + executable */
	MPU_REGION_ENTRY(
		"rom_region",
		CONFIG_FLASH_BASE_ADDRESS,
		REGION_FLASH_SIZE,
		{.rasr = P_RO_U_RO_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_NON_SHAREABLE}),
	/* RAM contains R/W data, non-executable */
#else /* !CONFIG_XIP */
	/* .text and .rodata are in RAM, flash is data only -> RO + XN */
	MPU_REGION_ENTRY(
		"flash",
		CONFIG_FLASH_BASE_ADDRESS,
		REGION_FLASH_SIZE,
		{.rasr = P_RO_U_RO_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_NON_SHAREABLE |
			 NOT_EXEC}),
	/* add rom_region mapping for SRAM which is RO + executable */
	MPU_REGION_ENTRY(
		"rom_region",
		(uint32_t)(&__rom_region_start),
		(uint32_t)(&__rom_region_mpu_size_bits),
		{.rasr = P_RO_U_RO_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE}),
#endif /* CONFIG_XIP */
	MPU_REGION_ENTRY(
		"peripherals",
		0xf8000000,
		REGION_128M,
		{.rasr = P_RW_U_NA_Msk |
			 DEVICE_SHAREABLE |
			 NOT_EXEC}),
#if (DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_ocm), okay))
	MPU_REGION_ENTRY(
		"ocm",
		DT_REG_ADDR(DT_CHOSEN(zephyr_ocm)),
		REGION_256K,
		{.rasr = FULL_ACCESS_Msk |
			 STRONGLY_ORDERED_SHAREABLE |
			 NOT_EXEC}),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
