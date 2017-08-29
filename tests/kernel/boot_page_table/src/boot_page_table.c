/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <mmustructs.h>
#include <ztest.h>
#include "boot_page_table.h"

#define MEMORY_REG_NUM 4

MMU_BOOT_REGION(START_ADDR_RANGE1, ADDR_SIZE, REGION_PERM);
MMU_BOOT_REGION(START_ADDR_RANGE2, ADDR_SIZE, REGION_PERM);
MMU_BOOT_REGION(START_ADDR_RANGE3, ADDR_SIZE, REGION_PERM);
MMU_BOOT_REGION(START_ADDR_RANGE4, ADDR_SIZE, REGION_PERM);

static int check_param(union x86_mmu_pae_pte *value, uint32_t perm)
{
	u32_t status = (value->rw  == ((perm & MMU_PTE_RW_MASK) >> 0x1));

	status &= (value->us == ((perm & MMU_PTE_US_MASK) >> 0x2));
	status &= value->p;
	return status;
}

static int check_param_nonset_region(union x86_mmu_pae_pte *value,
				     uint32_t perm)
{
	u32_t status = (value->rw  == 0);

	status &= (value->us == 0);
	status &= (value->p == 0);
	return status;
}

static int  starting_addr_range(u32_t start_addr_range)
{

	u32_t addr_range, status = true;
	union x86_mmu_pae_pte *value;

	for (addr_range = start_addr_range; addr_range <=
	     (start_addr_range + STARTING_ADDR_RANGE_LMT);
	       addr_range += 0x1000) {
		value = X86_MMU_GET_PTE(addr_range);
		status &= check_param(value, REGION_PERM);
		if (status == 0) {
			printk("error at %d permissions %d\n",
				addr_range, REGION_PERM);
			TC_PRINT("%s failed\n", __func__);
			return TC_FAIL;
		}
	}
	return TC_PASS;
}

static int before_start_addr_range(u32_t start_addr_range)
{
	u32_t addr_range, status = true;
	union x86_mmu_pae_pte *value;

	for (addr_range = start_addr_range - 0x7000;
	     addr_range < (start_addr_range); addr_range += 0x1000) {

		value = X86_MMU_GET_PTE(addr_range);
		status &= check_param_nonset_region(value,
						    REGION_PERM);
		if (status == 0) {
			TC_PRINT("%s failed\n", __func__);
			printk("error at %d permissions %d\n",
				addr_range, REGION_PERM);
			return TC_FAIL;

		}
	}
	return TC_PASS;
}

static int ending_start_addr_range(u32_t start_addr_range)
{
	u32_t addr_range, status = true;
	union x86_mmu_pae_pte *value;

	for (addr_range = start_addr_range + ADDR_SIZE; addr_range <
	     (start_addr_range + ADDR_SIZE + 0x10000);
	       addr_range += 0x1000) {
		value = X86_MMU_GET_PTE(addr_range);
		status &= check_param_nonset_region(value, REGION_PERM);
		if (status == 0) {
			TC_PRINT("%s failed\n", __func__);
			printk("error at %d permissions %d\n",
				addr_range, REGION_PERM);
			return TC_FAIL;

		}
	}
	return TC_PASS;
}

int boot_page_table(void)
{
	u32_t start_addr_range;
	int iterator = 0, check = 0;

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
		check = starting_addr_range(start_addr_range);
		if (check == TC_FAIL)
			return TC_FAIL;
		check = before_start_addr_range(start_addr_range);
		if (check == TC_FAIL)
			return TC_FAIL;
		check = ending_start_addr_range(start_addr_range);
		if (check == TC_FAIL)
			return TC_FAIL;

	}
	return TC_PASS;
}

void test_boot_page_table(void)
{
	zassert_true(boot_page_table() == TC_PASS, NULL);
}
