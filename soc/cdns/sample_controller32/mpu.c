/*
 * Copyright (c) 2024, 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <xtensa/config/core-isa.h>

#include <zephyr/devicetree.h>
#include <zephyr/arch/xtensa/mpu.h>
#include <zephyr/sys/util.h>

#define PHYS_SRAM0_ADDR (DT_REG_ADDR(DT_NODELABEL(sram0)))
#define PHYS_SRAM0_SIZE (DT_REG_SIZE(DT_NODELABEL(sram0)))

#define PHYS_ROM0_ADDR  (DT_REG_ADDR(DT_NODELABEL(srom0)))
#define PHYS_ROM0_SIZE  (DT_REG_SIZE(DT_NODELABEL(srom0)))

const struct xtensa_mpu_mem_type_region xtensa_mpu_mem_type_ranges[] = {
	{
		.start = 0x00000000,
		.end   = 0x3FFC0000,
		.memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
	},
	{
		/* dram*.bss */
		.start = 0x3FFC0000,
		.end   = 0x40000000,
		/* non-cacheable memory, non-shareable, non-bufferable, interruptible */
		.memory_type = 0x18,
	},
	{
		/* Vectors, vector helpers, ROM and RAM */
		.start = 0x40000000,
		.end   = PHYS_SRAM0_ADDR + PHYS_SRAM0_SIZE,
		/* non-cacheable memory, non-shareable, non-bufferable, interruptible */
		.memory_type = 0x18,
	},
	{
		.start = PHYS_SRAM0_ADDR + PHYS_SRAM0_SIZE,
		.end = 0xFFFFFFFF,
		.memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
	},
};

const unsigned int xtensa_mpu_mem_type_ranges_num = ARRAY_SIZE(xtensa_mpu_mem_type_ranges);
