/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 Lexmark International, Inc.
 * Copyright (c) 2025 Immo Birnbaum
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

extern const uint32_t __rom_region_start;
extern const uint32_t __rom_region_mpu_size_bits;
#ifdef CONFIG_NOCACHE_MEMORY
extern const uint32_t _nocache_ram_start;
extern const uint32_t _nocache_mpu_size_bits;
#endif /* CONFIG_NOCACHE_MEMORY */

static const struct arm_mpu_region mpu_regions[] = {
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
	/*
	 * Add rom_region mapping for SRAM which is RO + executable
	 * must be located at a higher MPU region index than sram as the rom_region
	 * is nested within sram in non-XIP mode and the attributes for this subset
	 * of sram must take precedence over those of the outer sram region.
	 */
	MPU_REGION_ENTRY(
		"rom_region",
		(uint32_t)(&__rom_region_start),
		(uint32_t)(&__rom_region_mpu_size_bits),
		{.rasr = P_RO_U_RO_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE}),
#endif /* CONFIG_XIP */
#ifdef CONFIG_NOCACHE_MEMORY
	/* Same as above applies: nocache located within sram -> higher region index reqd. */
	MPU_REGION_ENTRY(
		"nocache",
		(uint32_t)(&_nocache_ram_start),
		(uint32_t)(&_nocache_mpu_size_bits),
		{.rasr = P_RW_U_NA_Msk |
			 NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE |
			 NOT_EXEC}),
#endif /* CONFIG_NOCACHE_MEMORY */
	/*
	 * The address of the vectors is determined by arch/arm/core/cortex_a_r/prep_c.c
	 * -> for v7-R, there's no other option than 0x0, HIVECS always gets cleared.
	 * This region can potentially overlap with sram if the RAM base address is 0x0.
	 * The execute permissions are crucial for the system to work -> assign highest
	 * region index of any region (potentially) overlapping with the outer sram entry.
	 */
	MPU_REGION_ENTRY(
		"vectors",
		0x00000000,
		REGION_64B,
		{.rasr = P_RO_U_NA_Msk |
			 NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE}),
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
