/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/mem_manage.h>
#include <mmu.h>

#ifdef CONFIG_BACKING_STORE_RAM_PAGES
#define EXTRA_PAGES	(CONFIG_BACKING_STORE_RAM_PAGES - 1)
#else
#error "Unsupported configuration"
#endif

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

/* The mapped anonymous area will be free RAM plus half of the available
 * frames in the backing store.
 */
#define HALF_PAGES	(EXTRA_PAGES / 2)
#define HALF_BYTES	(HALF_PAGES * CONFIG_MMU_PAGE_SIZE)
static const char *nums = "0123456789";

void test_map_anon_pages(void)
{
	arena_size = k_mem_free_get() + HALF_BYTES;
	arena = k_mem_map(arena_size, K_MEM_PERM_RW);

	zassert_not_null(arena, "failed to map anonymous memory arena size %zu",
			 arena_size);
	printk("Anonymous memory arena %p size %zu\n", arena, arena_size);
	z_page_frames_dump();
}

void test_touch_anon_pages(void)
{
	unsigned long faults;

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
		arena[i] = 0;
	}

	faults = z_num_pagefaults_get() - faults;

	/* Specific number depends on how much RAM we have but shouldn't be 0 */
	zassert_not_equal(faults, 0UL, "no page faults handled?");
	printk("Kernel handled %lu page faults\n", faults);
}

void test_k_mem_page_out(void)
{
	unsigned long faults;
	int key, ret;

	/* Lock IRQs to prevent other pagefaults from happening while we
	 * are measuring stuff
	 */
	key = irq_lock();
	faults = z_num_pagefaults_get();
	ret = k_mem_page_out(arena, HALF_BYTES);
	zassert_equal(ret, 0, "k_mem_page_out failed with %d", ret);

	/* Write to the supposedly evicted region */
	for (size_t i = 0; i < HALF_BYTES; i++) {
		arena[i] = nums[i % 10];
	}
	faults = z_num_pagefaults_get() - faults;
	irq_unlock(key);

	zassert_equal(faults, HALF_PAGES,
		      "unexpected num pagefaults expected %lu got %d",
		      HALF_PAGES, faults);

	ret = k_mem_page_out(arena, arena_size);
	zassert_equal(ret, -ENOMEM, "k_mem_page_out should have failed");

}

void test_k_mem_page_in(void)
{
	unsigned long faults;
	int key, ret;

	/* Lock IRQs to prevent other pagefaults from happening while we
	 * are measuring stuff
	 */
	key = irq_lock();

	ret = k_mem_page_out(arena, HALF_BYTES);
	zassert_equal(ret, 0, "k_mem_page_out failed with %d", ret);

	k_mem_page_in(arena, HALF_BYTES);

	faults = z_num_pagefaults_get();
	/* Write to the supposedly evicted region */
	for (size_t i = 0; i < HALF_BYTES; i++) {
		arena[i] = nums[i % 10];
	}
	faults = z_num_pagefaults_get() - faults;
	irq_unlock(key);

	zassert_equal(faults, 0, "%d page faults when 0 expected",
		      faults);
}

void test_k_mem_pin(void)
{
	unsigned long faults;
	int key;

	k_mem_pin(arena, HALF_BYTES);

	/* Write to the rest of the arena */
	for (size_t i = HALF_BYTES; i < arena_size; i++) {
		arena[i] = nums[i % 10];
	}

	key = irq_lock();
	/* Show no faults writing to the pinned area */
	faults = z_num_pagefaults_get();
	for (size_t i = 0; i < HALF_BYTES; i++) {
		arena[i] = nums[i % 10];
	}
	faults = z_num_pagefaults_get() - faults;
	irq_unlock(key);

	zassert_equal(faults, 0, "%d page faults when 0 expected",
		      faults);

	/* Clean up */
	k_mem_unpin(arena, HALF_BYTES);
}

void test_k_mem_unpin(void)
{
	/* Pin the memory (which we know works from prior test) */
	k_mem_pin(arena, HALF_BYTES);

	/* Now un-pin it */
	k_mem_unpin(arena, HALF_BYTES);

	/* repeat the page_out scenario, which should work */
	test_k_mem_page_out();
}

/* Show that even if we map enough anonymous memory to fill the backing
 * store, we can still handle pagefaults.
 * This eats up memory so should be last in the suite.
 */
void test_backing_store_capacity(void)
{
	char *mem, *ret;
	int key;
	unsigned long faults;
	size_t size = (((CONFIG_BACKING_STORE_RAM_PAGES - 1) - HALF_PAGES) *
		       CONFIG_MMU_PAGE_SIZE);

	/* Consume the rest of memory */
	mem = k_mem_map(size, K_MEM_PERM_RW);
	zassert_not_null(mem, "k_mem_map failed");

	/* Show no memory is left */
	ret = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
	zassert_is_null(ret, "k_mem_map shouldn't have succeeded");

	key = irq_lock();
	faults = z_num_pagefaults_get();
	/* Poke all anonymous memory */
	for (size_t i = 0; i < HALF_BYTES; i++) {
		arena[i] = nums[i % 10];
	}
	for (size_t i = 0; i < size; i++) {
		mem[i] = nums[i % 10];
	}
	faults = z_num_pagefaults_get() - faults;
	irq_unlock(key);

	zassert_not_equal(faults, 0, "should have had some pagefaults");
}

/* ztest main entry*/
void test_main(void)
{
	ztest_test_suite(test_demand_paging,
			ztest_unit_test(test_map_anon_pages),
			ztest_unit_test(test_touch_anon_pages),
			ztest_unit_test(test_k_mem_page_out),
			ztest_unit_test(test_k_mem_page_in),
			ztest_unit_test(test_k_mem_pin),
			ztest_unit_test(test_k_mem_unpin),
			ztest_unit_test(test_backing_store_capacity));

	ztest_run_test_suite(test_demand_paging);
}
