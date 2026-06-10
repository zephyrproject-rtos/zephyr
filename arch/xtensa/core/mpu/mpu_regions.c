/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/xtensa/arch_inlines.h>
#include <zephyr/arch/xtensa/mpu.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>

#include <xtensa/corebits.h>
#include <xtensa/config/core-matmap.h>
#include <xtensa/config/core-isa.h>
#include <xtensa_mpu_priv.h>

extern char _heap_end[];
extern char _heap_start[];

/**
 * Static definition of all code and data memory regions of the
 * current Zephyr image. This information must be available and
 * need to be processed upon MPU initialization.
 */
__weak const struct xtensa_mpu_range xtensa_mpu_ranges[] = {
	/* Region for vector handlers. */
	{
		.start = (uintptr_t)XCHAL_VECBASE_RESET_VADDR,
		/*
		 * There is nothing from the Xtensa overlay about how big
		 * the vector handler region is. So we make an assumption
		 * that vecbase and .text are contiguous.
		 *
		 * SoC can override as needed if this is not the case,
		 * especially if the SoC reset/startup code relocates
		 * vecbase.
		 */
		.end   = (uintptr_t)__text_region_start,
		.access_rights = XTENSA_MPU_ACCESS_P_RX_U_RX,
	},
	/*
	 * Mark the zephyr execution regions (data, bss, noinit, etc.)
	 * cacheable, read / write and non-executable
	 */
	{
		/* This includes .data, .bss and various kobject sections. */
		.start = (uintptr_t)_image_ram_start,
		.end   = (uintptr_t)_image_ram_end,
		.access_rights = XTENSA_MPU_ACCESS_P_RW_U_NA,
	},
#if K_HEAP_MEM_POOL_SIZE > 0
	/* System heap memory */
	{
		.start = (uintptr_t)_heap_start,
		.end   = (uintptr_t)_heap_end,
		.access_rights = XTENSA_MPU_ACCESS_P_RW_U_NA,
	},
#endif
	/* Mark text segment cacheable, read only and executable */
	{
		.start = (uintptr_t)__text_region_start,
		.end   = (uintptr_t)__text_region_end,
		.access_rights = XTENSA_MPU_ACCESS_P_RX_U_RX,
	},
	/* Mark rodata segment cacheable, read only and non-executable */
	{
		.start = (uintptr_t)__rodata_region_start,
		.end   = (uintptr_t)__rodata_region_end,
		.access_rights = XTENSA_MPU_ACCESS_P_RO_U_RO,
	},
};

__weak const unsigned int xtensa_mpu_ranges_num = ARRAY_SIZE(xtensa_mpu_ranges);

/*
 * By default whole memory has the same type.
 * Override this in SoC or board layer if needed.
 */
__weak const struct xtensa_mpu_mem_type_region xtensa_mpu_mem_type_ranges[] = {
	{
		.start = 0x00000000,
		.end = 0xFFFFFFFF,
		.memory_type = CONFIG_XTENSA_MPU_DEFAULT_MEM_TYPE,
	},
};

__weak const unsigned int xtensa_mpu_mem_type_ranges_num = ARRAY_SIZE(xtensa_mpu_mem_type_ranges);
