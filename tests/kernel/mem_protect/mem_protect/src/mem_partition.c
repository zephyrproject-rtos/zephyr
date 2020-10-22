/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/mempool.h>
#include "mem_protect.h"

K_APPMEM_PARTITION_DEFINE(part2);
static K_APP_DMEM(part2) int part2_var = 1356;
static K_APP_BMEM(part2) int part2_zeroed_var = 20420;
static K_APP_BMEM(part2) int part2_bss_var;

SYS_MEM_POOL_DEFINE(data_pool, NULL, BLK_SIZE_MIN_MD, BLK_SIZE_MAX_MD,
		    BLK_NUM_MAX_MD, BLK_ALIGN_MD, K_APP_DMEM_SECTION(part2));

/**
 * @brief Test system provide means to obtain names of the data and BSS sections
 *
 * @details
 * - Define memory partition and define system memory pool using macros
 *   SYS_MEM_POOL_DEFINE
 * - Section name of the destination binary section for pool data will be
 *   obtained at build time by macros K_APP_DMEM_SECTION() which obtaines
 *   a section name.
 * - Then to check that system memory pool initialized correctly by allocating
 *   a block from it and check that it is not NULL.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see K_APP_DMEM_SECTION()
 */
void test_macros_obtain_names_data_bss(void)
{
	sys_mem_pool_init(&data_pool);
	void *block;

	block = sys_mem_pool_alloc(&data_pool, BLK_SIZE_MAX_MD - DESC_SIZE);
	zassert_not_null(block, NULL);
}

/**
 * @brief Test assigning global data and BSS variables to memory partitions
 *
 * @details Test that system supports application assigning global data and BSS
 * variables using macros K_APP_BMEM() and K_APP_DMEM
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_part_assign_bss_vars_zero(void)
{
	uint32_t read_data;
	/* The global variable part2_var will be inside the bounds of part2
	 * and be initialized with 1356 at boot.
	 */
	read_data = part2_var;
	zassert_true(read_data == 1356, NULL);

	/* The global variable part2_zeroed_var will be inside the bounds of
	 * part2 and must be zeroed at boot size K_APP_BMEM() was used,
	 * indicating a BSS variable.
	 */
	read_data = part2_zeroed_var;
	zassert_true(read_data == 0, NULL);

	/* The global variable part2_var will be inside the bounds of
	 * part2 and must be zeroed at boot size K_APP_BMEM() was used,
	 * indicating a BSS variable.
	 */
	read_data = part2_bss_var;
	zassert_true(read_data == 0, NULL);
}

K_APPMEM_PARTITION_DEFINE(part_arch);
K_APP_BMEM(part_arch) uint8_t __aligned(MEM_REGION_ALLOC) \
buf_arc[MEM_REGION_ALLOC];

/**
 * @brief Test partitions sized per the constraints of the MPU hardware
 *
 * @details
 * - Test that system automatically determine memory partition size according
 *   to the constraints of the platform's MPU hardware.
 * - Different platforms like x86, ARM and ARC have different MPU hardware for
 *   memory management.
 * - That test checks that MPU hardware works as expected and gives for the
 *   memory partition the most suitable and possible size.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_part_auto_determ_size_per_mpu(void)
{
	zassert_true(part_arch.size == MEM_REGION_ALLOC, NULL);
}
