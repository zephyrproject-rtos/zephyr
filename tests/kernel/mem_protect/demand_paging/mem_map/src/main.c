/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <zephyr/timing/timing.h>
#include <mmu.h>
#include <zephyr/linker/sections.h>

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

static bool expect_fault;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);

	if (expect_fault && reason == 0) {
		expect_fault = false;
		ztest_test_pass();
	} else {
		printk("Unexpected fault during test");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}

/* The mapped anonymous area will be free RAM plus half of the available
 * frames in the backing store.
 */
#define HALF_PAGES	(EXTRA_PAGES / 2)
#define HALF_BYTES	(HALF_PAGES * CONFIG_MMU_PAGE_SIZE)
static const char *nums = "0123456789";

/**
 * @brief Verify that more anonymous memory than free RAM can be mapped.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Demand paging is what lets a mapping exceed physical memory: the arena
 * requested here is deliberately larger than the free RAM by half the backing
 * store, so it can only be satisfied if the kernel is prepared to page. The
 * arena created here is the fixture every later case in the suite works on.
 *
 * Test steps:
 * - Query the free memory and request an arena that much plus half the
 *   spare backing-store capacity.
 * - Map it with k_mem_map().
 *
 * Expected result:
 * - The over-committed mapping succeeds and returns a usable arena.
 *
 * @see k_mem_map()
 */
ZTEST(demand_paging, test_map_anon_pages)
{
	arena_size = k_mem_free_get() + HALF_BYTES;
	arena = k_mem_map(arena_size, K_MEM_PERM_RW);

	zassert_not_null(arena, "failed to map anonymous memory arena size %zu",
			 arena_size);
	printk("Anonymous memory arena %p size %zu\n", arena, arena_size);
	k_mem_page_frames_dump();
}

static void print_paging_stats(struct k_mem_paging_stats_t *stats, const char *scope)
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

static void touch_anon_pages(bool zig, bool zag)
{
	void **arena_ptr = (void **)arena;
	size_t arena_ptr_size = arena_size / sizeof(void *);
	unsigned long faults;
	struct k_mem_paging_stats_t stats;
	k_tid_t tid = k_current_get();

	faults = k_mem_num_pagefaults_get();

	printk("checking zeroes\n");
	/* The mapped area should have started out zeroed. Check this. */
	for (size_t j = 0; j < arena_size; j++) {
		size_t i = zig ? (arena_size - 1 - j) : j;

		zassert_equal(arena[i], '\x00',
			      "page not zeroed got 0x%hhx at index %zu",
			      arena[i], i);
	}

	printk("writing data\n");
	/* Fill the whole arena with each location's own virtual address */
	for (size_t j = 0; j < arena_ptr_size; j++) {
		size_t i = zag ? (arena_ptr_size - 1 - j) : j;

		arena_ptr[i] = &arena_ptr[i];
	}

	/* And ensure it can be read back */
	printk("verify written data\n");
	for (size_t j = 0; j < arena_ptr_size; j++) {
		size_t i = zig ? (arena_ptr_size - 1 - j) : j;

		zassert_equal(arena_ptr[i], &arena_ptr[i],
			      "arena corrupted at index %zu: got %p expected %p",
			      i, arena_ptr[i], &arena_ptr[i]);
	}

	faults = k_mem_num_pagefaults_get() - faults;

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
	for (size_t j = 0; j < arena_ptr_size; j++) {
		size_t i = zag ? (arena_ptr_size - 1 - j) : j;

		zassert_equal(arena_ptr[i], &arena_ptr[i],
			      "arena corrupted at index %zu: got %p expected %p",
			      i, arena_ptr[i], &arena_ptr[i]);
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

/**
 * @brief Verify that touching the whole arena pages in, evicts and preserves
 *        data.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Walking an arena larger than RAM forces the full demand paging cycle:
 * never-touched pages arrive zeroed, writes fault pages in and push dirty
 * pages out to the backing store, and everything written must read back
 * intact after having been evicted and restored. The page fault counter and
 * the paging statistics -- global and per thread -- confirm the machinery
 * actually ran instead of the arena silently fitting in RAM.
 *
 * Test steps:
 * - Read the whole arena and check every byte is zero.
 * - Write each location's own address into it, then read it all back.
 * - Check the fault count is non-zero and dirty evictions occurred.
 * - Read the (now unmodified) arena again and check clean evictions occurred.
 * - Check the same counters in the current thread's statistics.
 *
 * Expected result:
 * - All data survives eviction and page-in, and the fault and eviction
 *   statistics show the paging happened.
 *
 * @see k_mem_paging_stats_get()
 * @see k_mem_paging_thread_stats_get()
 * @see k_mem_num_pagefaults_get()
 */
ZTEST(demand_paging, test_touch_anon_pages)
{
	touch_anon_pages(false, false);
}

/**
 * @brief Verify demand paging with reads and writes in opposing directions.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The same full-arena walk as the linear case, but reading the arena
 * backwards while writing it forwards. Traversal direction changes which
 * pages are resident when each access lands, so an eviction policy that only
 * survives sequential access is caught here.
 *
 * Test steps:
 * - Run the arena walk with reads descending and writes ascending.
 *
 * Expected result:
 * - Identical to the linear case: data survives and the paging statistics
 *   show faults and evictions.
 *
 * @see k_mem_paging_stats_get()
 */
ZTEST(demand_paging, test_touch_anon_pages_zigzag1)
{
	touch_anon_pages(true, false);
}

/**
 * @brief Verify demand paging with the opposite zig-zag pattern.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The mirror of the first zig-zag case: reads ascend while writes descend,
 * covering the remaining combination of traversal directions.
 *
 * Test steps:
 * - Run the arena walk with reads ascending and writes descending.
 *
 * Expected result:
 * - Identical to the linear case: data survives and the paging statistics
 *   show faults and evictions.
 *
 * @see k_mem_paging_stats_get()
 */
ZTEST(demand_paging, test_touch_anon_pages_zigzag2)
{
	touch_anon_pages(false, true);
}

/**
 * @brief Verify that unmapping a demand-paged region revokes access.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * k_mem_unmap() on a demand-paged arena has to tear down the whole region,
 * including pages currently paged out, so any later access faults rather than
 * resurrecting stale data. The suite's fatal error handler converts the
 * expected fault into a pass.
 *
 * Test steps:
 * - Unmap the arena with k_mem_unmap().
 * - Write to the first byte of the former arena.
 *
 * Expected result:
 * - The access faults; the code after it is never reached.
 *
 * @see k_mem_unmap()
 */
ZTEST(demand_paging, test_unmap_anon_pages)
{
	 k_mem_unmap(arena, arena_size);

	 /* memory should no longer be accessible */
	 expect_fault = true;
	 compiler_barrier();

	 TC_PRINT("Accessing unmapped memory should fault\n");
	 arena[0] = 'x';

	 /* and execution should not reach this point */
	 ztest_test_fail();
}

static void test_k_mem_page_out(void)
{
	unsigned long faults;
	int key, ret;

	/* Lock IRQs to prevent other pagefaults from happening while we
	 * are measuring stuff
	 */
	key = irq_lock();
	faults = k_mem_num_pagefaults_get();
	ret = k_mem_page_out(arena, HALF_BYTES);
	zassert_equal(ret, 0, "k_mem_page_out failed with %d", ret);

	/* Write to the supposedly evicted region */
	for (size_t i = 0; i < HALF_BYTES; i++) {
		arena[i] = nums[i % 10];
	}
	faults = k_mem_num_pagefaults_get() - faults;
	irq_unlock(key);

	zassert_equal(faults, HALF_PAGES,
		      "unexpected num pagefaults expected %d got %lu",
		      HALF_PAGES, faults);

	ret = k_mem_page_out(arena, arena_size);
	zassert_equal(ret, -ENOMEM, "k_mem_page_out should have failed");

}

/**
 * @brief Verify that k_mem_page_in() makes a region resident.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Preemptively paging a region in must leave every page resident, so
 * touching it afterwards causes no faults at all -- that absence, measured
 * with interrupts locked so nothing else can fault in between, is the
 * observable effect of the call.
 *
 * Test steps:
 * - Page half the arena out with k_mem_page_out().
 * - Page the same range back in with k_mem_page_in().
 * - With interrupts locked, write the whole range and count the faults.
 *
 * Expected result:
 * - Zero page faults are taken while writing the paged-in range.
 *
 * @see k_mem_page_in()
 * @see k_mem_page_out()
 */
ZTEST(demand_paging_api, test_k_mem_page_in)
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

	faults = k_mem_num_pagefaults_get();
	/* Write to the supposedly evicted region */
	for (size_t i = 0; i < HALF_BYTES; i++) {
		arena[i] = nums[i % 10];
	}
	faults = k_mem_num_pagefaults_get() - faults;
	irq_unlock(key);

	zassert_equal(faults, 0, "%lu page faults when 0 expected",
		      faults);
}

/**
 * @brief Verify that k_mem_pin() keeps a region resident under pressure.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * A pinned region must stay resident no matter how much paging traffic the
 * rest of the arena generates. The rest of the arena, which is larger than
 * RAM, is written first to force evictions; the pinned range must then be
 * writable without a single fault.
 *
 * Test steps:
 * - Pin half the arena with k_mem_pin().
 * - Write the rest of the over-committed arena to force eviction pressure.
 * - With interrupts locked, write the pinned range and count the faults.
 * - Unpin the range.
 *
 * Expected result:
 * - Zero page faults are taken in the pinned range despite the pressure.
 *
 * @see k_mem_pin()
 */
ZTEST(demand_paging_api, test_k_mem_pin)
{
	unsigned long faults;
	unsigned int key;

	k_mem_pin(arena, HALF_BYTES);

	/* Write to the rest of the arena */
	for (size_t i = HALF_BYTES; i < arena_size; i++) {
		arena[i] = nums[i % 10];
	}

	key = irq_lock();
	/* Show no faults writing to the pinned area */
	faults = k_mem_num_pagefaults_get();
	for (size_t i = 0; i < HALF_BYTES; i++) {
		arena[i] = nums[i % 10];
	}
	faults = k_mem_num_pagefaults_get() - faults;
	irq_unlock(key);

	zassert_equal(faults, 0, "%lu page faults when 0 expected",
		      faults);

	/* Clean up */
	k_mem_unpin(arena, HALF_BYTES);
}

/**
 * @brief Verify that k_mem_unpin() makes a region evictable again.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Unpinning must fully undo the pin: the region becomes an ordinary evictable
 * range again. That is shown by re-running the page-out scenario on it --
 * evicting the range succeeds, writing it back faults once per page, and
 * paging out more than the backing store can hold still fails with -ENOMEM.
 *
 * Test steps:
 * - Pin and then unpin half the arena.
 * - Page the range out and check it succeeds.
 * - With interrupts locked, write the range and count one fault per page.
 * - Attempt to page out the whole over-committed arena.
 *
 * Expected result:
 * - The unpinned range pages out and faults back in normally, and the
 *   oversized page-out fails with -ENOMEM.
 *
 * @see k_mem_unpin()
 * @see k_mem_page_out()
 */
ZTEST(demand_paging_api, test_k_mem_unpin)
{
	/* Pin the memory (which we know works from prior test) */
	k_mem_pin(arena, HALF_BYTES);

	/* Now un-pin it */
	k_mem_unpin(arena, HALF_BYTES);

	/* repeat the page_out scenario, which should work */
	test_k_mem_page_out();
}

/**
 * @brief Verify that paging still works with the backing store nearly full.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Filling almost the entire backing store must not wedge the paging
 * machinery: page faults have to keep being served even when eviction has
 * nowhere spare to go. Without CONFIG_DEMAND_MAPPING a further mapping must
 * be refused outright; with it, the mapping succeeds because pages are only
 * committed when touched. This case consumes the remaining memory, so it
 * runs in the suite that executes last.
 *
 * Test steps:
 * - Map enough anonymous memory to fill the backing store except one page.
 * - Attempt one more page mapping and check the configuration-dependent
 *   outcome.
 * - With interrupts locked, write both the old arena and the new mapping and
 *   count the faults.
 *
 * Expected result:
 * - The writes complete with page faults still being handled at capacity.
 *
 * @see k_mem_map()
 * @see k_mem_num_pagefaults_get()
 */
ZTEST(demand_paging_stat, test_backing_store_capacity)
{
	char *mem, *ret;
	unsigned int key;
	unsigned long faults;
	size_t size = (((CONFIG_BACKING_STORE_RAM_PAGES - 1) - HALF_PAGES) *
		       CONFIG_MMU_PAGE_SIZE);

	/* Consume the rest of memory */
	mem = k_mem_map(size, K_MEM_PERM_RW);
	zassert_not_null(mem, "k_mem_map failed");

	if (!IS_ENABLED(CONFIG_DEMAND_MAPPING)) {
		/* Show no memory is left */
		ret = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
		zassert_is_null(ret, "k_mem_map shouldn't have succeeded");
	} else {
		/* Show it doesn't matter */
		ret = k_mem_map(CONFIG_MMU_PAGE_SIZE, K_MEM_PERM_RW);
		zassert_not_null(ret, "k_mem_map should have succeeded");
	}

	key = irq_lock();
	faults = k_mem_num_pagefaults_get();
	/* Poke all anonymous memory */
	for (size_t i = 0; i < HALF_BYTES; i++) {
		arena[i] = nums[i % 10];
	}
	for (size_t i = 0; i < size; i++) {
		mem[i] = nums[i % 10];
	}
	faults = k_mem_num_pagefaults_get() - faults;
	irq_unlock(key);

	zassert_not_equal(faults, 0, "should have had some pagefaults");
}

/**
 * @brief Verify that paging statistics are readable from user mode.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The statistics queries are system calls, so a user thread must be able to
 * read both the global and its own per-thread paging counters, and the values
 * seen through the syscall path must reflect the paging activity earlier
 * cases generated rather than coming back empty.
 *
 * Test steps:
 * - From a user thread, read the global statistics with
 *   k_mem_paging_stats_get().
 * - Read the calling thread's statistics with
 *   k_mem_paging_thread_stats_get().
 * - Check the fault and eviction counters in both are non-zero.
 *
 * Expected result:
 * - Both queries succeed from user mode and report the paging activity that
 *   already took place.
 *
 * @see k_mem_paging_stats_get()
 * @see k_mem_paging_thread_stats_get()
 */
ZTEST_USER(demand_paging_stat, test_user_get_stats)
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
static bool print_histogram(struct k_mem_paging_histogram_t *hist)
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

/**
 * @brief Verify that paging timing histograms are readable from user mode.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * With CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM the kernel bins the time spent
 * evicting, paging in and paging out. Each histogram query is a system call,
 * and after the paging traffic of the earlier cases every histogram must
 * contain samples.
 *
 * Test steps:
 * - From a user thread, read the eviction, page-in and page-out histograms.
 * - Check each histogram has at least one non-zero bin.
 *
 * Expected result:
 * - All three histograms are readable from user mode and carry samples.
 *
 * @see k_mem_paging_histogram_eviction_get()
 * @see k_mem_paging_histogram_backing_store_page_in_get()
 * @see k_mem_paging_histogram_backing_store_page_out_get()
 */
ZTEST_USER(demand_paging_stat, test_user_get_hist)
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

void *demand_paging_api_setup(void)
{
	arena = k_mem_map(arena_size, K_MEM_PERM_RW);
	if (IS_ENABLED(CONFIG_DEMAND_MAPPING)) {
		/* force pages in */
		k_mem_page_in(arena, arena_size);
	}
	test_k_mem_page_out();

	return NULL;
}

ZTEST_SUITE(demand_paging, NULL, NULL, NULL, NULL, NULL);

ZTEST_SUITE(demand_paging_api, NULL, demand_paging_api_setup,
		NULL, NULL, NULL);

ZTEST_SUITE(demand_paging_stat, NULL, NULL, NULL, NULL, NULL);
