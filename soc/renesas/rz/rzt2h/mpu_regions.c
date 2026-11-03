/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-defs.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

#define ATCM_NODE      DT_NODELABEL(atcm)
#define BTCM_NODE      DT_NODELABEL(btcm)
#define SRAM0_NODE     DT_NODELABEL(sram0)
#define SRAM1_NODE     DT_NODELABEL(sram1)
#define EXT_SPACE_NODE DT_NODELABEL(ext_space)
#define ATCM_AXIS_NODE DT_NODELABEL(cpu0_atcm_axis)
#define BTCM_AXIS_NODE DT_NODELABEL(cpu0_btcm_axis)

#define DEVICE_REGION_START 0x80000000UL
#define DEVICE_REGION_LIMIT 0xA0000000UL

#define REGION_START(node_id) DT_REG_ADDR(node_id)
#define REGION_LIMIT(node_id) (DT_REG_ADDR(node_id) + DT_REG_SIZE(node_id))

#define REGION_EXECUTABLE_RAM_ATTR(limit)                                                          \
	{                                                                                          \
		.rbar = FULL_ACCESS_Msk | NON_SHAREABLE_Msk, /* AP, SH */                          \
		.mair_idx = MPU_MAIR_INDEX_SRAM,             /* Cacheable */                       \
		.r_limit = limit - 1                         /* Region Limit */                    \
	}

/* From soc/renesas/rz/scripts/linker.ld */
extern char __btcm_text_end[];
extern char __atcm_text_start[];
extern char __atcm_text_end[];

static const struct arm_mpu_region mpu_regions[] = {
	/* Peripherals, GIC */
	MPU_REGION_ENTRY("DEVICE", DEVICE_REGION_START, REGION_DEVICE_ATTR(DEVICE_REGION_LIMIT)),

	/* BTCM holds the loader program, the kernel text and the kernel stacks */
	MPU_REGION_ENTRY("BTCM_TEXT", REGION_START(BTCM_NODE),
			 REGION_RAM_TEXT_ATTR((uintptr_t)__btcm_text_end)),

	MPU_REGION_ENTRY("BTCM", (uintptr_t)__btcm_text_end,
			 REGION_EXECUTABLE_RAM_ATTR(REGION_LIMIT(BTCM_NODE))),

	/* ATCM holds the vector table, the critical routines and the CPU mode stacks */
	MPU_REGION_ENTRY("VECTORS", (uintptr_t)_vector_start,
			 REGION_RAM_TEXT_ATTR((uintptr_t)__atcm_text_start)),

	MPU_REGION_ENTRY("ATCM_TEXT", (uintptr_t)__atcm_text_start,
			 REGION_RAM_TEXT_ATTR((uintptr_t)__atcm_text_end)),

	MPU_REGION_ENTRY("ATCM", (uintptr_t)__atcm_text_end,
			 REGION_EXECUTABLE_RAM_ATTR(REGION_LIMIT(ATCM_NODE))),

	/*
	 * Lower half of the system SRAM, left to CPU1. J-Link runs its flash
	 * loader from it (64 KiB at 0x10000000), so it is executable, otherwise
	 * flashing fails.
	 */
	MPU_REGION_ENTRY("SRAM1", REGION_START(SRAM1_NODE),
			 REGION_EXECUTABLE_RAM_ATTR(REGION_LIMIT(SRAM1_NODE))),

	/* RAM */
	MPU_REGION_ENTRY("TEXT", (uintptr_t)__text_region_start,
			 REGION_RAM_TEXT_ATTR((uintptr_t)__rodata_region_start)),

	MPU_REGION_ENTRY("RODATA", (uintptr_t)__rodata_region_start,
			 REGION_RAM_RO_ATTR((uintptr_t)__rodata_region_end)),

	MPU_REGION_ENTRY("SRAM0", (uintptr_t)__rodata_region_end,
			 REGION_RAM_ATTR(REGION_LIMIT(SRAM0_NODE))),

	MPU_REGION_ENTRY("EXT_SPACE", REGION_START(EXT_SPACE_NODE),
			 REGION_EXECUTABLE_RAM_ATTR(REGION_LIMIT(EXT_SPACE_NODE))),

	MPU_REGION_ENTRY("ATCM_AXIS", REGION_START(ATCM_AXIS_NODE),
			 REGION_EXECUTABLE_RAM_ATTR(REGION_LIMIT(ATCM_AXIS_NODE))),

	MPU_REGION_ENTRY("BTCM_AXIS", REGION_START(BTCM_AXIS_NODE),
			 REGION_EXECUTABLE_RAM_ATTR(REGION_LIMIT(BTCM_AXIS_NODE))),

};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
