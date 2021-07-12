/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/mem_manage.h>
#include <timing/timing.h>
#include <mmu.h>
#include <linker/sections.h>

#ifdef CONFIG_BACKING_STORE_RAM_PAGES
#define EXTRA_PAGES	(CONFIG_BACKING_STORE_RAM_PAGES - 1)
#else
#error "Unsupported configuration"
#endif

#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
#ifdef CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS

#ifdef CONFIG_BOARD_QEMU_X86_TINY
unsigned long
k_mem_paging_eviction_histogram_bounds[
	CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS] = {
	10000,
	20000,
	30000,
	40000,
	50000,
	60000,
	70000,
	80000,
	100000,
	ULONG_MAX
};

unsigned long
k_mem_paging_backing_store_histogram_bounds[
	CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS] = {
	10000,
	50000,
	100000,
	150000,
	200000,
	250000,
	500000,
	750000,
	1000000,
	ULONG_MAX
};
#else
#error "Need to define paging histogram bounds"
#endif

#endif /* CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS */
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */

size_t arena_size;
char *arena;

__pinned_bss
static bool expect_fault;

__pinned_func
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

void print_paging_stats(struct k_mem_paging_stats_t *stats, const char *scope)
{
	printk("* Page Faults (%s):\n", scope);
	printk("    - Total: %lu\n", stats->pagefaults.cnt);
	printk("    - IRQ locked: %lu\n", stats->pagefaults.irq_locked);
	printk("    - IRQ unlocked: %lu\n", stats->pagefaults.irq_unlocked);
#ifndef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	printk("    - in ISR: %lu\n", stats->pagefaults.in_isr);
#endif

	printk("* Eviction (%s):\n", scope);
	printk("    - Total pages evicted: %lu\n",
	       stats->eviction.clean + stats->eviction.dirty);
	printk("    - Clean pages evicted: %lu\n",
	       stats->eviction.clean);
	printk("    - Dirty pages evicted: %lu\n",
	       stats->eviction.dirty);
}

void test_touch_anon_pages(void)
{
	unsigned long faults;
	struct k_mem_paging_stats_t stats;
	k_tid_t tid = k_current_get();

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

	/* And ensure it can be read back */
	printk("verify written data\n");
	for (size_t i = 0; i < arena_size; i++) {
		zassert_equal(arena[i], nums[i % 10],
			      "arena corrupted at index %d (%p): got 0x%hhx expected 0x%hhx",
			      i, &arena[i], arena[i], nums[i % 10]);
	}

	faults = z_num_pagefaults_get() - faults;

	/* Specific number depends on how much RAM we have but shouldn't be 0 */
	zassert_not_equal(faults, 0UL, "no page faults handled?");
	printk("Kernel handled %lu page faults\n", faults);

	k_mem_paging_stats_get(&stats);
	print_paging_stats(&stats, "kernel");
	zassert_not_equal(stats.eviction.dirty, 0UL,
			  "there should be dirty pages being evicted.");

#ifdef CONFIG_EVICTION_NRU
	k_msleep(CONFIG_EVICTION_NRU_PERIOD * 2);
#endif /* CONFIG_EVICTION_NRU */

	/* There should be some clean pages to be evicted now,
	 * since the arena is not modified.
	 */
	printk("reading unmodified data\n");
	for (size_t i = 0; i < arena_size; i++) {
		zassert_equal(arena[i], nums[i % 10],
			      "arena corrupted at index %d (%p): got 0x%hhx expected 0x%hhx",
			      i, &arena[i], arena[i], nums[i % 10]);
	}

	k_mem_paging_stats_get(&stats);
	print_paging_stats(&stats, "kernel");
	zassert_not_equal(stats.eviction.clean, 0UL,
			  "there should be clean pages being evicted.");

	/* per-thread statistics */
	printk("\nPaging stats for current thread (%p):\n", tid);
	k_mem_paging_thread_stats_get(tid, &stats);
	print_paging_stats(&stats, "thread");
	zassert_not_equal(stats.pagefaults.cnt, 0UL,
			  "no page faults handled in thread?");
	zassert_not_equal(stats.eviction.dirty, 0UL,
			  "test thread should have dirty pages evicted.");
	zassert_not_equal(stats.eviction.clean, 0UL,
			  "test thread should have clean pages evicted.");

	/* Reset arena to zero */
	for (size_t i = 0; i < arena_size; i++) {
		arena[i] = 0;
	}
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

/* Test if we can get paging statistics under usermode */
void test_user_get_stats(void)
{
	struct k_mem_paging_stats_t stats;
	k_tid_t tid = k_current_get();

	/* overall kernel statistics */
	printk("\nPaging stats for kernel:\n");
	k_mem_paging_stats_get(&stats);
	print_paging_stats(&stats, "kernel - usermode");
	zassert_not_equal(stats.pagefaults.cnt, 0UL,
			  "no page faults handled in thread?");
	zassert_not_equal(stats.eviction.dirty, 0UL,
			  "test thread should have dirty pages evicted.");
	zassert_not_equal(stats.eviction.clean, 0UL,
			  "test thread should have clean pages evicted.");

	/* per-thread statistics */
	printk("\nPaging stats for current thread (%p):\n", tid);
	k_mem_paging_thread_stats_get(tid, &stats);
	print_paging_stats(&stats, "thread - usermode");
	zassert_not_equal(stats.pagefaults.cnt, 0UL,
			  "no page faults handled in thread?");
	zassert_not_equal(stats.eviction.dirty, 0UL,
			  "test thread should have dirty pages evicted.");
	zassert_not_equal(stats.eviction.clean, 0UL,
			  "test thread should have clean pages evicted.");

}

/* Print the histogram and return true if histogram has non-zero values
 * in one of its bins.
 */
bool print_histogram(struct k_mem_paging_histogram_t *hist)
{
	bool has_non_zero;
	uint64_t time_ns;
	int idx;

	has_non_zero = false;
	for (idx = 0;
	     idx < CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS;
	     idx++) {
#ifdef CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS
		time_ns = timing_cycles_to_ns(hist->bounds[idx]);
#else
		time_ns = k_cyc_to_ns_ceil64(hist->bounds[idx]);
#endif
		printk("  <= %llu ns (%lu cycles): %lu\n", time_ns,
		       hist->bounds[idx], hist->counts[idx]);
		if (hist->counts[idx] > 0U) {
			has_non_zero = true;
		}
	}

	return has_non_zero;
}

/* Test if we can get paging timing histograms */
void test_user_get_hist(void)
{
	struct k_mem_paging_histogram_t hist;

	printk("Eviction Timing Histogram:\n");
	k_mem_paging_histogram_eviction_get(&hist);
	zassert_true(print_histogram(&hist),
		     "should have non-zero counts in histogram.");
	printk("\n");

	printk("Backing Store Page-IN Histogram:\n");
	k_mem_paging_histogram_backing_store_page_in_get(&hist);
	zassert_true(print_histogram(&hist),
		     "should have non-zero counts in histogram.");
	printk("\n");

	printk("Backing Store Page-OUT Histogram:\n");
	k_mem_paging_histogram_backing_store_page_out_get(&hist);
	zassert_true(print_histogram(&hist),
		     "should have non-zero counts in histogram.");
	printk("\n");
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
			ztest_unit_test(test_backing_store_capacity),
			ztest_user_unit_test(test_user_get_stats),
			ztest_user_unit_test(test_user_get_hist));

	ztest_run_test_suite(test_demand_paging);
}
