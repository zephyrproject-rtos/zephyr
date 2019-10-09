/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <arch/x86/mmustructs.h>
#include <ztest.h>
#include "boot_page_table.h"

#define MEMORY_REG_NUM 4

MMU_BOOT_REGION(START_ADDR_RANGE1, ADDR_SIZE, REGION_PERM);
MMU_BOOT_REGION(START_ADDR_RANGE2, ADDR_SIZE, REGION_PERM);
MMU_BOOT_REGION(START_ADDR_RANGE3, ADDR_SIZE, REGION_PERM);
MMU_BOOT_REGION(START_ADDR_RANGE4, ADDR_SIZE, REGION_PERM);

static int check_param(u64_t value, u64_t perm)
{
	return ((value & REGION_PERM) == REGION_PERM);
}

static int check_param_nonset_region(u64_t value, u64_t perm)
{
	u32_t status = ((value & Z_X86_MMU_RW) == 0);

	status &= ((value & Z_X86_MMU_US) == 0);
	status &= ((value & Z_X86_MMU_P) == 0);
	return status;
}

static void starting_addr_range(u32_t start_addr_range)
{

	u32_t addr_range, status = true;
	u64_t value;

	for (addr_range = start_addr_range; addr_range <=
	     (start_addr_range + STARTING_ADDR_RANGE_LMT);
	     addr_range += 0x1000) {
		value = *z_x86_get_pte(&z_x86_kernel_ptables, addr_range);
		status &= check_param(value, REGION_PERM);
		zassert_false((status == 0U), "error at %d permissions %d\n",
			      addr_range, REGION_PERM);
	}
}

static void before_start_addr_range(u32_t start_addr_range)
{
	u32_t addr_range, status = true;
	u64_t value;

	for (addr_range = start_addr_range - 0x7000;
	     addr_range < (start_addr_range); addr_range += 0x1000) {

		value = *z_x86_get_pte(&z_x86_kernel_ptables, addr_range);
		status &= check_param_nonset_region(value, REGION_PERM);

		zassert_false((status == 0U), "error at %d permissions %d\n",
			      addr_range, REGION_PERM);
	}
}

static void ending_start_addr_range(u32_t start_addr_range)
{
	u32_t addr_range, status = true;
	u64_t value;

	for (addr_range = start_addr_range + ADDR_SIZE; addr_range <
	     (start_addr_range + ADDR_SIZE + 0x10000);
	     addr_range += 0x1000) {
		value = *z_x86_get_pte(&z_x86_kernel_ptables, addr_range);
		status &= check_param_nonset_region(value, REGION_PERM);
		zassert_false((status == 0U), "error at %d permissions %d\n",
			      addr_range, REGION_PERM);
	}
}
/**
 * @brief Test for boot page table
 * @defgroup kernel_boot_page_table_tests Boot Page Table
 * @ingroup all_tests
 * @{
 */

/**
 * @brief Test boot page table entry permissions
 *
 * @details Initiallize a memory region with particular permission.
 * Later using the same address read the corresponding page table entry.
 * And using the PTE check the permission of the region, it should match.
 * Permission of the memory region is validated just before the specified
 * region and just after the specified region.
 */
void test_boot_page_table(void)
{
	u32_t start_addr_range;
	int iterator = 0;

	for (iterator = 0; iterator < MEMORY_REG_NUM; iterator++) {
		switch (iterator) {

		case 0:
			start_addr_range = START_ADDR_RANGE1;
			break;

		case 1:
			start_addr_range = START_ADDR_RANGE2;
			break;

		case 2:
			start_addr_range = START_ADDR_RANGE3;
			break;

		case 3:
			start_addr_range = START_ADDR_RANGE4;
			break;
		default:
			break;
		}
		starting_addr_range(start_addr_range);
		before_start_addr_range(start_addr_range);
		ending_start_addr_range(start_addr_range);

	}
}

/**
 * @}
 */
