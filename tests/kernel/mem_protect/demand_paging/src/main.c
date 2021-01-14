/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/mem_manage.h>
#include <mmu.h>

#define SWAP_PAGES	16
size_t arena_size;
char *arena;

static bool expect_fault;

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);

	if (expect_fault && reason == 0) {
		expect_fault = false;
		ztest_test_pass();
	} else {
		printk("Unexpected fault during test");
		k_fatal_halt(reason);
	}
}

void test_map_anon_pages(void)
{
	arena_size = k_mem_free_get() + ((SWAP_PAGES - 1) * CONFIG_MMU_PAGE_SIZE);
	arena = k_mem_map(arena_size, K_MEM_PERM_RW);

	zassert_not_null(arena, "failed to map anonymous memory arena size %zu",
			 arena_size);
	printk("Anonymous memory arena %p size %zu\n", arena, arena_size);
	z_page_frames_dump();
}

void test_touch_anon_pages(void)
{
	unsigned long faults;
	static const char *nums = "0123456789";

	faults = z_num_pagefaults_get();

	printk("checking zeroes\n");
	/* The mapped area should have started out zeroed. Check this. */
	for (size_t i = 0; i < arena_size; i++) {
		zassert_equal(arena[i], '\x00',
			      "page not zeroed got 0x%hhx at index %d",
			      arena[i], i);
	}

	printk("writing data\n");
	/* Write a pattern of data to the whole arena */
	for (size_t i = 0; i < arena_size; i++) {
		arena[i] = nums[i % 10];
	}

	printk("reading data\n");
	/* And ensure it can be read back */
	for (size_t i = 0; i < arena_size; i++) {
		zassert_equal(arena[i], nums[i % 10],
			      "arena corrupted at index %d (%p): got 0x%hhx expected 0x%hhx",
			      i, &arena[i], arena[i], nums[i % 10]);
	}

	faults = z_num_pagefaults_get() - faults;

	/* Specific number depends on how much RAM we have but shouldn't be 0 */
	zassert_not_equal(faults, 0UL, "no page faults handled?");
	printk("Kernel handled %lu page faults\n", faults);
}

/* ztest main entry*/
void test_main(void)
{
	ztest_test_suite(test_demand_paging,
			ztest_unit_test(test_map_anon_pages),
			ztest_unit_test(test_touch_anon_pages)
			);
	ztest_run_test_suite(test_demand_paging);
}
