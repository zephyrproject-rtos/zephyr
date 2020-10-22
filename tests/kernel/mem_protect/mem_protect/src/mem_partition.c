/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/mempool.h>
#include "mem_protect.h"

static K_APP_DMEM(ztest_mem_partition) int var = 1356;
static K_APP_BMEM(ztest_mem_partition) int zeroed_var = 20420;
static K_APP_BMEM(ztest_mem_partition) int bss_var;

SYS_MEM_POOL_DEFINE(data_pool, NULL, BLK_SIZE_MIN_MD, BLK_SIZE_MAX_MD,
		    BLK_NUM_MAX_MD, BLK_ALIGN_MD,
		    K_APP_DMEM_SECTION(ztest_mem_partition));

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
	/* The global variable var will be inside the bounds of
	 * ztest_mem_partition and be initialized with 1356 at boot.
	 */
	zassert_true(var == 1356, NULL);

	/* The global variable zeroed_var will be inside the bounds of
	 * ztest_mem_partition and must be zeroed at boot size K_APP_BMEM() was
	 * used, indicating a BSS variable.
	 */
	zassert_true(zeroed_var == 0, NULL);

	/* The global variable var will be inside the bounds of
	 * ztest_mem_partition and must be zeroed at boot size K_APP_BMEM() was
	 * used, indicating a BSS variable.
	 */
	zassert_true(bss_var == 0, NULL);
}

K_APPMEM_PARTITION_DEFINE(part_arch);
K_APP_BMEM(part_arch) uint8_t __aligned(MEM_REGION_ALLOC)
	buf_arc[MEM_REGION_ALLOC];

/**
 * @brief Test partitions sized per the constraints of the MPU hardware
 *
 * @details
 * - MEM_REGION_ALLOC is pre-sized to naturally fit in the target hardware's
 *   memory management granularity. Show that the partition size matches.
 * - Show that the base address of the partition is properly set, it should
 *   match the base address of buf_arc.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_mem_part_auto_determ_size(void)
{
	zassert_true(part_arch.size == MEM_REGION_ALLOC, NULL);
	zassert_true(part_arch.start == (uintptr_t)buf_arc,
	   "Base address of memory partition not determined at build time");
}
