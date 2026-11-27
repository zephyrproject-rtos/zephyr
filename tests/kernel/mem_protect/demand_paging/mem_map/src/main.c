/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/app_memory/mem_domain.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <zephyr/spinlock.h>
#include <zephyr/timing/timing.h>
#include <mmu.h>
#include <kernel_arch_interface.h>
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

/**
 * @brief Verify that unmapping never-touched pages keeps the backing store
 *        consistent.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * With CONFIG_DEMAND_MAPPING, pages of a new anonymous mapping hold a special
 * location token until first touched and own no backing store slot.
 * Unmapping them must not release a slot. The backing store is then cycled
 * by over-committing RAM again, which must still page out and in correctly.
 * Skipped without CONFIG_DEMAND_MAPPING.
 *
 * Test steps:
 * - Map anonymous memory and unmap it without touching it.
 * - Map more anonymous memory than free RAM, write every page and read every
 *   page back.
 * - Unmap it.
 *
 * Expected result:
 * - No fault or assertion, and all data reads back intact.
 *
 * @see k_mem_map()
 * @see k_mem_unmap()
 */
ZTEST(demand_paging, test_unmap_untouched_anon_pages)
{
	size_t size;
	char *mem;

	if (!IS_ENABLED(CONFIG_DEMAND_MAPPING)) {
		ztest_test_skip();
	}

	mem = k_mem_map(HALF_BYTES, K_MEM_PERM_RW);
	zassert_not_null(mem, "failed to map untouched memory");
	k_mem_unmap(mem, HALF_BYTES);

	size = k_mem_free_get() + HALF_BYTES;
	mem = k_mem_map(size, K_MEM_PERM_RW);
	zassert_not_null(mem, "failed to map memory size %zu", size);

	for (size_t i = 0; i < size; i += CONFIG_MMU_PAGE_SIZE) {
		mem[i] = nums[(i / CONFIG_MMU_PAGE_SIZE) % 10];
	}
	for (size_t i = 0; i < size; i += CONFIG_MMU_PAGE_SIZE) {
		zassert_equal(mem[i], nums[(i / CONFIG_MMU_PAGE_SIZE) % 10],
			      "data corrupted at offset %zu", i);
	}

	k_mem_unmap(mem, size);
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

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS >= 2)
#define SMP_RACE_STACK_SIZE    (2048 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define SMP_RACE_DURATION_MS   1500
#define SMP_CONCURRENT_ITERS   200U
#define MAX_ACTIVE_SMP_THREADS 2U

static K_THREAD_STACK_DEFINE(smp_worker_0_stack, SMP_RACE_STACK_SIZE);
static K_THREAD_STACK_DEFINE(smp_worker_1_stack, SMP_RACE_STACK_SIZE);

static struct k_thread *active_smp_threads[MAX_ACTIVE_SMP_THREADS];
static size_t active_smp_thread_count;
static atomic_t *active_smp_stop;
static struct k_sem *active_smp_sem;
static unsigned int active_smp_sem_count;

struct smp_race_ctx {
	void *page;
	atomic_t ready;
	atomic_t stop;
	atomic_t race_found;
	atomic_t page_outs;
	atomic_t writer_active;
	atomic_t evictor_active;
	atomic_t concurrent_overlaps;
	atomic_t writer_cpu;
	atomic_t evictor_cpu;
};

static void smp_writer_race_entry(void *p1, void *p2, void *p3)
{
	struct smp_race_ctx *ctx = p1;
	volatile uint32_t *word = (volatile uint32_t *)ctx->page;
	uint32_t iter = 0U;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_set(&ctx->writer_cpu, (atomic_val_t)arch_curr_cpu()->id);
	atomic_inc(&ctx->ready);

	while (atomic_get(&ctx->ready) < 2) {
		if (atomic_get(&ctx->stop) != 0) {
			return;
		}
		arch_nop();
	}

	while ((atomic_get(&ctx->stop) == 0) && (atomic_get(&ctx->race_found) == 0)) {
		atomic_set(&ctx->writer_active, 1);
		if (atomic_get(&ctx->evictor_active) != 0) {
			atomic_inc(&ctx->concurrent_overlaps);
		}
		*word = iter;
		iter++;
		atomic_set(&ctx->writer_active, 0);

		if ((iter & 0x0fU) == 0U) {
			k_spinlock_key_t mm_key = k_spin_lock(z_mm_lock_get());
			uintptr_t phys = 0U;
			uintptr_t flags = arch_page_info_get(ctx->page, &phys, false);
			bool mismatch = false;

			if ((flags & ARCH_DATA_PAGE_LOADED) != 0U) {
				if (!k_mem_is_page_frame(phys)) {
					mismatch = true;
				} else {
					struct k_mem_page_frame *pf =
						k_mem_phys_to_page_frame(phys);

					if (k_mem_page_frame_is_free(pf) ||
					    !k_mem_page_frame_is_mapped(pf) ||
					    (k_mem_page_frame_to_virt(pf) != ctx->page)) {
						mismatch = true;
					}
				}
			}
			k_spin_unlock(z_mm_lock_get(), mm_key);

			if (mismatch) {
				atomic_set(&ctx->race_found, 1);
				TC_PRINT("SMP race detected at iter=%u phys=0x%lx "
					 "page_outs=%ld\n",
					 iter, phys, (long)atomic_get(&ctx->page_outs));
				break;
			}
		}
	}
}

static void smp_evictor_entry(void *p1, void *p2, void *p3)
{
	struct smp_race_ctx *ctx = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_set(&ctx->evictor_cpu, (atomic_val_t)arch_curr_cpu()->id);
	atomic_inc(&ctx->ready);

	while (atomic_get(&ctx->ready) < 2) {
		if (atomic_get(&ctx->stop) != 0) {
			return;
		}
		arch_nop();
	}

	while ((atomic_get(&ctx->stop) == 0) && (atomic_get(&ctx->race_found) == 0)) {
		k_spinlock_key_t mm_key = k_spin_lock(z_mm_lock_get());
		uintptr_t flags_before = arch_page_info_get(ctx->page, NULL, false);

		k_spin_unlock(z_mm_lock_get(), mm_key);

		atomic_set(&ctx->evictor_active, 1);
		if (atomic_get(&ctx->writer_active) != 0) {
			atomic_inc(&ctx->concurrent_overlaps);
		}
		if ((k_mem_page_out(ctx->page, CONFIG_MMU_PAGE_SIZE) == 0) &&
		    ((flags_before & ARCH_DATA_PAGE_LOADED) != 0U)) {
			atomic_inc(&ctx->page_outs);
		}
		atomic_set(&ctx->evictor_active, 0);
	}
}

#define SMP_FAULT_WAIT_TIMEOUT_MS 10000

struct smp_fault_ctx {
	volatile uint8_t *page;
	struct k_sem start_sem;
	struct k_sem ready_sem;
	struct k_sem done_sem;
	atomic_t stop;
	atomic_t go;
	atomic_t writer_cpu;
	atomic_t reader_cpu;
	atomic_t writer_active;
	atomic_t reader_active;
	atomic_t concurrent_overlaps;
	uint8_t marker;
	uint8_t read_val;
};

static void smp_writer_fault_entry(void *p1, void *p2, void *p3)
{
	struct smp_fault_ctx *ctx = p1;
	atomic_val_t seq = 1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_set(&ctx->writer_cpu, (atomic_val_t)arch_curr_cpu()->id);

	for (;;) {
		(void)k_sem_take(&ctx->start_sem, K_FOREVER);
		if (atomic_get(&ctx->stop) != 0) {
			break;
		}
		k_sem_give(&ctx->ready_sem);
		while (atomic_get(&ctx->go) < seq) {
			if (atomic_get(&ctx->stop) != 0) {
				return;
			}
			arch_nop();
		}
		atomic_set(&ctx->writer_active, 1);
		if (atomic_get(&ctx->reader_active) != 0) {
			atomic_inc(&ctx->concurrent_overlaps);
		}
		ctx->page[1] = ctx->marker;
		atomic_set(&ctx->writer_active, 0);
		k_sem_give(&ctx->done_sem);
		seq++;
	}
}

static void smp_reader_entry(void *p1, void *p2, void *p3)
{
	struct smp_fault_ctx *ctx = p1;
	atomic_val_t seq = 1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_set(&ctx->reader_cpu, (atomic_val_t)arch_curr_cpu()->id);

	for (;;) {
		(void)k_sem_take(&ctx->start_sem, K_FOREVER);
		if (atomic_get(&ctx->stop) != 0) {
			break;
		}
		k_sem_give(&ctx->ready_sem);
		while (atomic_get(&ctx->go) < seq) {
			if (atomic_get(&ctx->stop) != 0) {
				return;
			}
			arch_nop();
		}
		atomic_set(&ctx->reader_active, 1);
		if (atomic_get(&ctx->writer_active) != 0) {
			atomic_inc(&ctx->concurrent_overlaps);
		}
		ctx->read_val = ctx->page[0];
		atomic_set(&ctx->reader_active, 0);
		k_sem_give(&ctx->done_sem);
		seq++;
	}
}

static void pin_and_start_pair(struct k_thread *thread0, k_thread_stack_t *stack0,
			       size_t stack0_size, k_thread_entry_t entry0, void *arg0,
			       struct k_thread *thread1, k_thread_stack_t *stack1,
			       size_t stack1_size, k_thread_entry_t entry1, void *arg1)
{
	(void)k_thread_create(thread0, stack0, stack0_size, entry0, arg0, NULL, NULL,
			      K_PRIO_PREEMPT(0), 0U, K_FOREVER);
	(void)k_thread_create(thread1, stack1, stack1_size, entry1, arg1, NULL, NULL,
			      K_PRIO_PREEMPT(0), 0U, K_FOREVER);

#if defined(CONFIG_SCHED_CPU_MASK)
	zassert_ok(k_thread_cpu_pin(thread0, 0), "failed to pin worker 0 to CPU 0");
	zassert_ok(k_thread_cpu_pin(thread1, 1), "failed to pin worker 1 to CPU 1");
#endif

	active_smp_threads[0] = thread0;
	active_smp_threads[1] = thread1;
	active_smp_thread_count = 2U;

	k_thread_start(thread0);
	k_thread_start(thread1);
}

static void cleanup_active_smp_threads(void)
{
	int first_err = 0;
	size_t remaining = 0U;

	if (active_smp_thread_count == 0U) {
		return;
	}

	if (active_smp_stop != NULL) {
		atomic_set(active_smp_stop, 1);
	}
	if (active_smp_sem != NULL) {
		for (unsigned int i = 0U; i < active_smp_sem_count; i++) {
			k_sem_give(active_smp_sem);
		}
	}

	for (size_t i = 0U; i < active_smp_thread_count; i++) {
		int ret = k_thread_join(active_smp_threads[i], K_SECONDS(10));

		if (ret != 0) {
			if (first_err == 0) {
				first_err = ret;
			}
			active_smp_threads[remaining++] = active_smp_threads[i];
		}
	}

	active_smp_thread_count = remaining;
	if (remaining == 0U) {
		active_smp_stop = NULL;
		active_smp_sem = NULL;
		active_smp_sem_count = 0U;
	}
	zassert_ok(first_err, "helper thread failed to join (%d)", first_err);
}
#endif

/**
 * @brief Verify SMP safety between demand paging faults and concurrent page-out.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * On SMP, a thread taking access-flag or dirty-tracking faults on a page must
 * serialize properly with another CPU concurrently evicting the same page via
 * k_mem_page_out(). Otherwise a stale descriptor write-back can resurrect a
 * valid mapping to a freed page frame or trip page-frame ownership assertions.
 *
 * Test steps:
 * - Spawn a helper thread that repeatedly calls k_mem_page_out() on a page.
 * - Concurrently write to the same page from the main thread and verify
 *   page-table and page-frame state consistency.
 *
 * Expected result:
 * - No assertion failure and no resurrected mapping to an unmapped page frame.
 *
 * @see k_mem_page_out()
 */
ZTEST(demand_paging_api, test_smp_page_out_race)
{
#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS >= 2)
	static struct k_thread writer_thread;
	static struct k_thread evictor_thread;
	static struct smp_race_ctx ctx;
	volatile uint32_t *word = (volatile uint32_t *)arena;
	unsigned long faults_before;
	int64_t deadline;

	if (arch_num_cpus() < 2U) {
		ztest_test_skip();
	}

	ctx.page = arena;
	atomic_set(&ctx.ready, 0);
	atomic_set(&ctx.stop, 0);
	atomic_set(&ctx.race_found, 0);
	atomic_set(&ctx.page_outs, 0);
	atomic_set(&ctx.writer_active, 0);
	atomic_set(&ctx.evictor_active, 0);
	atomic_set(&ctx.concurrent_overlaps, 0);
	atomic_set(&ctx.writer_cpu, -1);
	atomic_set(&ctx.evictor_cpu, -1);

	/* Ensure the victim page starts resident and a free backing slot exists. */
	k_mem_page_in(arena, CONFIG_MMU_PAGE_SIZE);
	*word = 0U;
	faults_before = k_mem_num_pagefaults_get();

	active_smp_stop = &ctx.stop;
	active_smp_sem = NULL;
	active_smp_sem_count = 0U;
	pin_and_start_pair(&writer_thread, smp_worker_0_stack,
			   K_THREAD_STACK_SIZEOF(smp_worker_0_stack), smp_writer_race_entry, &ctx,
			   &evictor_thread, smp_worker_1_stack,
			   K_THREAD_STACK_SIZEOF(smp_worker_1_stack), smp_evictor_entry, &ctx);

	deadline = k_uptime_get() + SMP_FAULT_WAIT_TIMEOUT_MS;
	while (atomic_get(&ctx.ready) < 2) {
		if (k_uptime_get() >= deadline) {
			zassert_unreachable("timed out waiting for SMP race threads to start");
		}
		k_msleep(1);
	}

	k_msleep(SMP_RACE_DURATION_MS);

	cleanup_active_smp_threads();
	zassert_equal(atomic_get(&ctx.race_found), 0, "stale PTE resurrected unmapped page frame");
	zassert_not_equal(atomic_get(&ctx.writer_cpu), atomic_get(&ctx.evictor_cpu),
			  "writer (CPU %ld) and evictor (CPU %ld) ran on same CPU",
			  (long)atomic_get(&ctx.writer_cpu), (long)atomic_get(&ctx.evictor_cpu));
	zassert_true(atomic_get(&ctx.page_outs) > 0, "no concurrent page_outs occurred");
	zassert_true((k_mem_num_pagefaults_get() - faults_before) > 0UL,
		     "no page faults triggered after concurrent page-out");
	zassert_true(atomic_get(&ctx.concurrent_overlaps) > 0,
		     "no cross-CPU overlap between writer and evictor observed");

	/* Restore clean backing-store state for subsequent capacity tests. */
	k_mem_page_in(arena, CONFIG_MMU_PAGE_SIZE);
#else
	ztest_test_skip();
#endif
}

/**
 * @brief Verify concurrent SMP read and write faults on the same paged-out page.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Two CPUs faulting concurrently on the same paged-out page both take a
 * translation fault; the second CPU to enter the handler sees the descriptor
 * already paged in. Likewise, concurrent AF and dirty updates must preserve the
 * dirty state across a subsequent page-out and page-in.
 *
 * Test steps:
 * - Evict or clear AF on the target page, synchronize pinned writer and reader
 *   threads on distinct CPUs at a barrier, then release them concurrently.
 * - Directly verify ARCH_DATA_PAGE_DIRTY under z_mm_lock, then page out and
 *   page back in to verify both persistence and that page-in clears dirty.
 *
 * Expected result:
 * - Neither thread takes a fatal fault, ARCH_DATA_PAGE_DIRTY is set, and the
 *   written byte survives eviction.
 *
 * @see k_mem_page_out()
 * @see k_mem_page_in()
 */
ZTEST(demand_paging_api, test_smp_concurrent_fault_race)
{
#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS >= 2)
	static struct k_thread writer_thread;
	static struct k_thread reader_thread;
	static struct smp_fault_ctx ctx;
	volatile uint8_t *page = (volatile uint8_t *)arena;

	if (arch_num_cpus() < 2U) {
		ztest_test_skip();
	}

	ctx.page = page;
	k_sem_init(&ctx.start_sem, 0U, 2U);
	k_sem_init(&ctx.ready_sem, 0U, 2U);
	k_sem_init(&ctx.done_sem, 0U, 2U);
	atomic_set(&ctx.stop, 0);
	atomic_set(&ctx.go, 0);
	atomic_set(&ctx.writer_cpu, -1);
	atomic_set(&ctx.reader_cpu, -1);
	atomic_set(&ctx.writer_active, 0);
	atomic_set(&ctx.reader_active, 0);
	atomic_set(&ctx.concurrent_overlaps, 0);
	ctx.marker = 0U;
	ctx.read_val = 0U;

	active_smp_stop = &ctx.stop;
	active_smp_sem = &ctx.start_sem;
	active_smp_sem_count = 2U;
	pin_and_start_pair(&writer_thread, smp_worker_0_stack,
			   K_THREAD_STACK_SIZEOF(smp_worker_0_stack), smp_writer_fault_entry, &ctx,
			   &reader_thread, smp_worker_1_stack,
			   K_THREAD_STACK_SIZEOF(smp_worker_1_stack), smp_reader_entry, &ctx);

	for (uint32_t i = 0U; i < SMP_CONCURRENT_ITERS; i++) {
		atomic_val_t seq = (atomic_val_t)(i + 1U);
		uint8_t marker = (uint8_t)(i + 1U);
		k_spinlock_key_t mm_key;
		uintptr_t flags;
		uintptr_t flags_after;

		page[0] = marker;
		page[1] = 0U;
		if ((i & 1U) == 0U) {
			/* Alternate between paged-out translation faults and loaded AF faults. */
			zassert_equal(k_mem_page_out(arena, CONFIG_MMU_PAGE_SIZE), 0,
				      "k_mem_page_out failed at iter %u", i);
		} else {
			zassert_equal(k_mem_page_out(arena, CONFIG_MMU_PAGE_SIZE), 0,
				      "k_mem_page_out failed at iter %u", i);
			k_mem_page_in(arena, CONFIG_MMU_PAGE_SIZE);
			mm_key = k_spin_lock(z_mm_lock_get());
			flags = arch_page_info_get(arena, NULL, true);
			flags_after = arch_page_info_get(arena, NULL, false);
			k_spin_unlock(z_mm_lock_get(), mm_key);
			zassert_true((flags & ARCH_DATA_PAGE_LOADED) != 0U,
				     "page not loaded after page-in at iter %u", i);
			zassert_equal(flags & ARCH_DATA_PAGE_DIRTY, 0U,
				      "freshly paged-in page unexpectedly dirty at iter %u", i);
			zassert_equal(flags_after & ARCH_DATA_PAGE_ACCESSED, 0U,
				      "AF bit was not cleared at iter %u", i);
		}

		ctx.marker = marker;
		ctx.read_val = 0U;
		k_sem_give(&ctx.start_sem);
		k_sem_give(&ctx.start_sem);

		zassert_ok(k_sem_take(&ctx.ready_sem, K_MSEC(SMP_FAULT_WAIT_TIMEOUT_MS)),
			   "workers ready timeout (1st) at iter %u", i);
		zassert_ok(k_sem_take(&ctx.ready_sem, K_MSEC(SMP_FAULT_WAIT_TIMEOUT_MS)),
			   "workers ready timeout (2nd) at iter %u", i);

		atomic_set(&ctx.go, seq);

		zassert_ok(k_sem_take(&ctx.done_sem, K_MSEC(SMP_FAULT_WAIT_TIMEOUT_MS)),
			   "workers done timeout (1st) at iter %u", i);
		zassert_ok(k_sem_take(&ctx.done_sem, K_MSEC(SMP_FAULT_WAIT_TIMEOUT_MS)),
			   "workers done timeout (2nd) at iter %u", i);

		zassert_equal(ctx.read_val, marker, "reader observed corrupted byte at iter %u", i);

		mm_key = k_spin_lock(z_mm_lock_get());
		flags = arch_page_info_get(arena, NULL, false);
		k_spin_unlock(z_mm_lock_get(), mm_key);
		zassert_true((flags & ARCH_DATA_PAGE_LOADED) != 0U,
			     "page not loaded after concurrent fault at iter %u", i);
		zassert_true((flags & ARCH_DATA_PAGE_DIRTY) != 0U,
			     "dirty bit lost after concurrent write at iter %u", i);

		zassert_equal(k_mem_page_out(arena, CONFIG_MMU_PAGE_SIZE), 0,
			      "post-write k_mem_page_out failed at iter %u", i);
		k_mem_page_in(arena, CONFIG_MMU_PAGE_SIZE);
		zassert_equal(page[1], marker, "dirty write lost after page-out/in at iter %u", i);

		mm_key = k_spin_lock(z_mm_lock_get());
		flags = arch_page_info_get(arena, NULL, false);
		k_spin_unlock(z_mm_lock_get(), mm_key);
		zassert_equal(flags & ARCH_DATA_PAGE_DIRTY, 0U,
			      "dirty bit not cleared after page-in at iter %u", i);
	}

	cleanup_active_smp_threads();
	zassert_not_equal(atomic_get(&ctx.writer_cpu), atomic_get(&ctx.reader_cpu),
			  "writer (CPU %ld) and reader (CPU %ld) ran on same CPU",
			  (long)atomic_get(&ctx.writer_cpu), (long)atomic_get(&ctx.reader_cpu));
	zassert_true(atomic_get(&ctx.concurrent_overlaps) > 0,
		     "no cross-CPU overlap between concurrent fault workers observed");
#else
	ztest_test_skip();
#endif
}

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS >= 2)
#if defined(CONFIG_USERSPACE) && defined(CONFIG_ARCH_MEM_DOMAIN_SUPPORTS_DEINIT)
#define HAS_SMP_PRIV_RACE 1
#endif
#endif

#if defined(HAS_SMP_PRIV_RACE)
struct smp_privatize_ctx {
	volatile char *page;
	struct k_mem_partition ptn;
	atomic_t ready;
	atomic_t stop;
	atomic_t race_found;
	atomic_t privatizes;
	atomic_t page_cycles;
	atomic_t domain_err;
	atomic_t pager_cpu;
	atomic_t domain_cpu;
};

static void smp_privatize_pager_entry(void *p1, void *p2, void *p3)
{
	struct smp_privatize_ctx *ctx = p1;
	uint32_t iter = 0U;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_set(&ctx->pager_cpu, (atomic_val_t)arch_curr_cpu()->id);
	atomic_inc(&ctx->ready);

	while (atomic_get(&ctx->ready) < 2) {
		if (atomic_get(&ctx->stop) != 0) {
			return;
		}
		arch_nop();
	}

	while ((atomic_get(&ctx->stop) == 0) && (atomic_get(&ctx->race_found) == 0)) {
		k_spinlock_key_t mm_key;
		uintptr_t phys = 0U;
		uintptr_t flags;
		char marker = (char)(iter & 0x7fU);
		bool mismatch = false;

		if (k_mem_page_out((void *)ctx->page, CONFIG_MMU_PAGE_SIZE) == 0) {
			ctx->page[0] = marker;
			if (ctx->page[0] != marker) {
				mismatch = true;
			}
			atomic_inc(&ctx->page_cycles);
		}
		mm_key = k_spin_lock(z_mm_lock_get());
		flags = arch_page_info_get((void *)ctx->page, &phys, true);
		if ((flags & ARCH_DATA_PAGE_LOADED) != 0U) {
			if (!k_mem_is_page_frame(phys)) {
				mismatch = true;
			} else {
				struct k_mem_page_frame *pf = k_mem_phys_to_page_frame(phys);

				if (k_mem_page_frame_is_free(pf) ||
				    !k_mem_page_frame_is_mapped(pf) ||
				    (k_mem_page_frame_to_virt(pf) != (void *)ctx->page)) {
					mismatch = true;
				}
			}
		}
		k_spin_unlock(z_mm_lock_get(), mm_key);

		if (mismatch) {
			atomic_set(&ctx->race_found, 1);
			break;
		}
		iter++;
	}
}

static void smp_privatize_domain_entry(void *p1, void *p2, void *p3)
{
	struct smp_privatize_ctx *ctx = p1;
	struct k_mem_partition *ptn_ptr = &ctx->ptn;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_set(&ctx->domain_cpu, (atomic_val_t)arch_curr_cpu()->id);
	atomic_inc(&ctx->ready);

	while (atomic_get(&ctx->ready) < 2) {
		if (atomic_get(&ctx->stop) != 0) {
			return;
		}
		arch_nop();
	}

	while ((atomic_get(&ctx->stop) == 0) && (atomic_get(&ctx->race_found) == 0)) {
		struct k_mem_domain dom;
		int ret = k_mem_domain_init(&dom, 1U, &ptn_ptr);

		if (ret != 0) {
			atomic_set(&ctx->domain_err, ret);
			break;
		}
		ret = k_mem_domain_deinit(&dom);
		if (ret != 0) {
			atomic_set(&ctx->domain_err, ret);
			break;
		}
		atomic_inc(&ctx->privatizes);
	}
}
#endif

/**
 * @brief Verify SMP safety between demand paging PTE updates and table privatization.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Duplicating shared translation tables during memory-domain initialization or
 * partition privatization updates source leaf descriptors (setting nG) under
 * xlat_lock. Concurrent demand-paging PTE updates (page-out, page-in, AF
 * clearing, and dirty updates) must serialize with the same lock so neither
 * path overwrites the other.
 */
ZTEST(demand_paging_api, test_smp_privatize_page_out_race)
{
#if defined(HAS_SMP_PRIV_RACE)
	static struct k_thread pager_thread;
	static struct k_thread domain_thread;
	static struct smp_privatize_ctx ctx;
	int64_t deadline;

	if (arch_num_cpus() < 2U) {
		ztest_test_skip();
	}

	k_mem_page_in(arena, CONFIG_MMU_PAGE_SIZE);
	arena[0] = 0x11;

	ctx = (struct smp_privatize_ctx){0};
	ctx.page = (volatile char *)arena;
	ctx.ptn.start = (uintptr_t)arena;
	ctx.ptn.size = CONFIG_MMU_PAGE_SIZE;
	ctx.ptn.attr = K_MEM_PARTITION_P_RW_U_RW;
	atomic_set(&ctx.pager_cpu, -1);
	atomic_set(&ctx.domain_cpu, -1);

	active_smp_stop = &ctx.stop;
	active_smp_sem = NULL;
	active_smp_sem_count = 0U;
	pin_and_start_pair(
		&pager_thread, smp_worker_0_stack, K_THREAD_STACK_SIZEOF(smp_worker_0_stack),
		smp_privatize_pager_entry, &ctx, &domain_thread, smp_worker_1_stack,
		K_THREAD_STACK_SIZEOF(smp_worker_1_stack), smp_privatize_domain_entry, &ctx);

	deadline = k_uptime_get() + SMP_FAULT_WAIT_TIMEOUT_MS;
	while (atomic_get(&ctx.ready) < 2) {
		if (k_uptime_get() >= deadline) {
			zassert_unreachable("timed out waiting for privatize race threads");
		}
		k_msleep(1);
	}

	k_msleep(SMP_RACE_DURATION_MS);
	cleanup_active_smp_threads();

	zassert_ok((int)atomic_get(&ctx.domain_err), "domain init/deinit failed (%ld)",
		   (long)atomic_get(&ctx.domain_err));
	zassert_equal(atomic_get(&ctx.race_found), 0,
		      "PTE/page-frame ownership corrupted during concurrent privatization");
	zassert_not_equal(atomic_get(&ctx.pager_cpu), atomic_get(&ctx.domain_cpu),
			  "pager (CPU %ld) and domain (CPU %ld) ran on same CPU",
			  (long)atomic_get(&ctx.pager_cpu), (long)atomic_get(&ctx.domain_cpu));
	zassert_true(atomic_get(&ctx.page_cycles) > 0, "no demand-paging cycles ran");
	zassert_true(atomic_get(&ctx.privatizes) > 0, "no domain privatizations ran");

	k_mem_page_in(arena, CONFIG_MMU_PAGE_SIZE);
#else
	ztest_test_skip();
#endif
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

static void demand_paging_api_after(void *data)
{
	ARG_UNUSED(data);

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS >= 2)
	cleanup_active_smp_threads();
#endif
}

ZTEST_SUITE(demand_paging, NULL, NULL, NULL, NULL, NULL);

ZTEST_SUITE(demand_paging_api, NULL, demand_paging_api_setup, NULL, demand_paging_api_after, NULL);

ZTEST_SUITE(demand_paging_stat, NULL, NULL, NULL, NULL, NULL);
