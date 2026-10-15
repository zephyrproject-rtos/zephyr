/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Demand paging: page faults, eviction, page-in, page-out and pinning. The
 * virtual memory core in kernel/mmu.c reaches this code through the hooks
 * declared in mmu.h.
 */

#include <stdint.h>
#include <kernel_arch_interface.h>
#include <zephyr/spinlock.h>
#include <mmu.h>
#include <kernel_internal.h>
#include <zephyr/kernel/mm/demand_paging.h>
#include <zephyr/sys/check.h>
#include <zephyr/timing/timing.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_DEMAND_PAGING_STATS
struct k_mem_paging_stats_t paging_stats;
extern struct k_mem_paging_histogram_t z_paging_histogram_eviction;
extern struct k_mem_paging_histogram_t z_paging_histogram_backing_store_page_in;
extern struct k_mem_paging_histogram_t z_paging_histogram_backing_store_page_out;
#endif /* CONFIG_DEMAND_PAGING_STATS */

static inline void do_backing_store_page_in(uintptr_t location)
{
#ifdef CONFIG_DEMAND_MAPPING
	/* Check for special cases */
	switch (location) {
	case ARCH_UNPAGED_ANON_ZERO:
		memset(K_MEM_SCRATCH_PAGE, 0, CONFIG_MMU_PAGE_SIZE);
		__fallthrough;
	case ARCH_UNPAGED_ANON_UNINIT:
		/* nothing else to do */
		return;
	default:
		break;
	}
#endif /* CONFIG_DEMAND_MAPPING */

#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
	uint32_t time_diff;

#ifdef CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS
	timing_t time_start, time_end;

	time_start = timing_counter_get();
#else
	uint32_t time_start;

	time_start = k_cycle_get_32();
#endif /* CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS */
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */

	k_mem_paging_backing_store_page_in(location);

#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
#ifdef CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS
	time_end = timing_counter_get();
	time_diff = (uint32_t)timing_cycles_get(&time_start, &time_end);
#else
	time_diff = k_cycle_get_32() - time_start;
#endif /* CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS */

	z_paging_histogram_inc(&z_paging_histogram_backing_store_page_in,
			       time_diff);
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */
}

static inline void do_backing_store_page_out(uintptr_t location)
{
#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
	uint32_t time_diff;

#ifdef CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS
	timing_t time_start, time_end;

	time_start = timing_counter_get();
#else
	uint32_t time_start;

	time_start = k_cycle_get_32();
#endif /* CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS */
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */

	k_mem_paging_backing_store_page_out(location);

#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
#ifdef CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS
	time_end = timing_counter_get();
	time_diff = (uint32_t)timing_cycles_get(&time_start, &time_end);
#else
	time_diff = k_cycle_get_32() - time_start;
#endif /* CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS */

	z_paging_histogram_inc(&z_paging_histogram_backing_store_page_out,
			       time_diff);
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */
}

#if defined(CONFIG_SMP) && defined(CONFIG_DEMAND_PAGING_ALLOW_IRQ)
/*
 * SMP support is very simple. Some resources such as the scratch page could
 * be made per CPU, backing store driver execution be confined to the faulting
 * CPU, statistics be made to cope with access concurrency, etc. But in the
 * end we're dealing with memory transfer to/from some external storage which
 * is inherently slow and whose access is most likely serialized anyway.
 * So let's simply enforce global demand paging serialization across all CPUs
 * with a mutex as there is no real gain from added parallelism here.
 */
static K_MUTEX_DEFINE(z_mm_paging_lock);
#endif

static void virt_region_foreach(void *addr, size_t size,
				void (*func)(void *vaddr))
{
	k_mem_assert_virtual_region(addr, size);

	for (size_t offset = 0; offset < size; offset += CONFIG_MMU_PAGE_SIZE) {
		func((uint8_t *)addr + offset);
	}
}

/*
 * Perform some preparatory steps before paging out. The provided page frame
 * must be evicted to the backing store immediately after this is called
 * with a call to k_mem_paging_backing_store_page_out() if it contains
 * a data page.
 *
 * - Map page frame to scratch area if requested. This always is true if we're
 *   doing a page fault, but is only set on manual evictions if the page is
 *   dirty.
 * - If mapped:
 *    - obtain backing store location and populate location parameter
 *    - Update page tables with location
 * - Mark page frame as busy
 *
 * Returns -ENOMEM if the backing store is full
 */
static int page_frame_prepare_locked(struct k_mem_page_frame *pf, bool *dirty_ptr,
				     bool page_fault, uintptr_t *location_ptr)
{
	uintptr_t phys;
	int ret;
	bool dirty = *dirty_ptr;

	phys = k_mem_page_frame_to_phys(pf);
	__ASSERT(!k_mem_page_frame_is_pinned(pf), "page frame 0x%lx is pinned",
		 phys);

	/* If the backing store doesn't have a copy of the page, even if it
	 * wasn't modified, treat as dirty. This can happen for a few
	 * reasons:
	 * 1) Page has never been swapped out before, and the backing store
	 *    wasn't pre-populated with this data page.
	 * 2) Page was swapped out before, but the page contents were not
	 *    preserved after swapping back in.
	 * 3) Page contents were preserved when swapped back in, but were later
	 *    evicted from the backing store to make room for other evicted
	 *    pages.
	 */
	if (k_mem_page_frame_is_mapped(pf)) {
		dirty = dirty || !k_mem_page_frame_is_backed(pf);
	}

	if (dirty || page_fault) {
		arch_mem_scratch(phys);
	}

	if (k_mem_page_frame_is_mapped(pf)) {
		ret = k_mem_paging_backing_store_location_get(pf, location_ptr,
							      page_fault);
		if (ret != 0) {
			LOG_ERR("out of backing store memory");
			return -ENOMEM;
		}
		arch_mem_page_out(k_mem_page_frame_to_virt(pf), *location_ptr);

		if (IS_ENABLED(CONFIG_EVICTION_TRACKING)) {
			k_mem_paging_eviction_remove(pf);
		}
	} else {
		/* Shouldn't happen unless this function is mis-used */
		__ASSERT(!dirty, "un-mapped page determined to be dirty");
	}
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	/* Mark as busy so that k_mem_page_frame_is_evictable() returns false */
	__ASSERT(!k_mem_page_frame_is_busy(pf), "page frame 0x%lx is already busy",
		 phys);
	k_mem_page_frame_set(pf, K_MEM_PAGE_FRAME_BUSY);
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	/* Update dirty parameter, since we set to true if it wasn't backed
	 * even if otherwise clean
	 */
	*dirty_ptr = dirty;

	return 0;
}

struct k_mem_page_frame *z_paging_frame_evict_locked(void)
{
	struct k_mem_page_frame *pf;
	uintptr_t location;
	bool dirty;
	int ret;

	pf = k_mem_paging_eviction_select(&dirty);
	if (pf == NULL) {
		/* Every evictable page frame is pinned or busy */
		return NULL;
	}
	LOG_DBG("evicting %p at 0x%lx",
		k_mem_page_frame_to_virt(pf),
		k_mem_page_frame_to_phys(pf));
	ret = page_frame_prepare_locked(pf, &dirty, false, &location);
	if (ret != 0) {
		return NULL;
	}
	if (dirty) {
		do_backing_store_page_out(location);
	}
	pf->va_and_flags = 0;

	return pf;
}

int z_paging_unmap_page_locate(void *addr, uintptr_t *phys, bool *paged_out)
{
	enum arch_page_location status;
	uintptr_t location;
	int ret;

	*paged_out = false;
	status = arch_page_location_get(addr, &location);
	switch (status) {
	case ARCH_PAGE_LOCATION_PAGED_OUT:
		/*
		 * No pf is associated with this mapping.
		 * Simply get rid of the MMU entry and free
		 * corresponding backing store.
		 */
		ret = arch_mem_unmap(addr, CONFIG_MMU_PAGE_SIZE);
		__ASSERT(ret == 0, "%s: arch_mem_unmap() returned %d", __func__,
			 ret);

		k_mem_paging_backing_store_location_free(location);
		*paged_out = true;
		return ret;
	case ARCH_PAGE_LOCATION_PAGED_IN:
		/*
		 * The page is in memory but it may not be
		 * accessible in order to manage tracking
		 * of the ARCH_DATA_PAGE_ACCESSED flag
		 * meaning arch_page_phys_get() could fail.
		 * Still, we know the actual phys address.
		 */
		*phys = location;
		return 0;
	default:
		return arch_page_phys_get(addr, phys);
	}
}

void z_paging_init(void)
{
	struct k_mem_page_frame *pf;
	uintptr_t phys;

#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
	z_paging_histogram_init();
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */
	k_mem_paging_backing_store_init();
	k_mem_paging_eviction_init();

	if (IS_ENABLED(CONFIG_EVICTION_TRACKING)) {
		/* start tracking evictable page installed above if any */
		K_MEM_PAGE_FRAME_FOREACH(phys, pf) {
			if (k_mem_page_frame_is_evictable(pf)) {
				k_mem_paging_eviction_add(pf);
			}
		}
	}
}

static int do_mem_evict(void *addr)
{
	bool dirty;
	struct k_mem_page_frame *pf;
	uintptr_t location;
	k_spinlock_key_t key;
	uintptr_t flags, phys;
	int ret;

#if CONFIG_DEMAND_PAGING_ALLOW_IRQ
	__ASSERT(!k_is_in_isr(),
		 "%s is unavailable in ISRs with CONFIG_DEMAND_PAGING_ALLOW_IRQ",
		 __func__);
#ifdef CONFIG_SMP
	k_mutex_lock(&z_mm_paging_lock, K_FOREVER);
#else
	k_sched_lock();
#endif
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	key = k_spin_lock(&z_mm_lock);
	flags = arch_page_info_get(addr, &phys, false);
	__ASSERT((flags & ARCH_DATA_PAGE_NOT_MAPPED) == 0,
		 "address %p isn't mapped", addr);
	if ((flags & ARCH_DATA_PAGE_LOADED) == 0) {
		/* Un-mapped or already evicted. Nothing to do */
		ret = 0;
		goto out;
	}

	dirty = (flags & ARCH_DATA_PAGE_DIRTY) != 0;
	pf = k_mem_phys_to_page_frame(phys);
	__ASSERT(k_mem_page_frame_to_virt(pf) == addr, "page frame address mismatch");
	ret = page_frame_prepare_locked(pf, &dirty, false, &location);
	if (ret != 0) {
		goto out;
	}

#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	k_spin_unlock(&z_mm_lock, key);
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	if (dirty) {
		do_backing_store_page_out(location);
	}
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	key = k_spin_lock(&z_mm_lock);
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	z_mem_page_frame_free_locked(pf);
out:
	k_spin_unlock(&z_mm_lock, key);
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
#ifdef CONFIG_SMP
	k_mutex_unlock(&z_mm_paging_lock);
#else
	k_sched_unlock();
#endif
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	return ret;
}

int k_mem_page_out(void *addr, size_t size)
{
	__ASSERT(z_mem_page_frames_initialized, "%s called on %p too early", __func__,
		 addr);
	k_mem_assert_virtual_region(addr, size);

	for (size_t offset = 0; offset < size; offset += CONFIG_MMU_PAGE_SIZE) {
		void *pos = (uint8_t *)addr + offset;
		int ret;

		ret = do_mem_evict(pos);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int k_mem_page_frame_evict(uintptr_t phys)
{
	k_spinlock_key_t key;
	struct k_mem_page_frame *pf;
	bool dirty;
	uintptr_t flags;
	uintptr_t location;
	int ret;

	__ASSERT(z_mem_page_frames_initialized, "%s called on 0x%lx too early",
		 __func__, phys);

	/* Implementation is similar to do_page_fault() except there is no
	 * data page to page-in, see comments in that function.
	 */

#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	__ASSERT(!k_is_in_isr(),
		 "%s is unavailable in ISRs with CONFIG_DEMAND_PAGING_ALLOW_IRQ",
		 __func__);
#ifdef CONFIG_SMP
	k_mutex_lock(&z_mm_paging_lock, K_FOREVER);
#else
	k_sched_lock();
#endif
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	key = k_spin_lock(&z_mm_lock);
	pf = k_mem_phys_to_page_frame(phys);
	if (!k_mem_page_frame_is_mapped(pf)) {
		/* Nothing to do, free page */
		ret = 0;
		goto out;
	}
	flags = arch_page_info_get(k_mem_page_frame_to_virt(pf), NULL, false);
	/* Shouldn't ever happen */
	__ASSERT((flags & ARCH_DATA_PAGE_LOADED) != 0, "data page not loaded");
	dirty = (flags & ARCH_DATA_PAGE_DIRTY) != 0;
	ret = page_frame_prepare_locked(pf, &dirty, false, &location);
	if (ret != 0) {
		goto out;
	}

#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	k_spin_unlock(&z_mm_lock, key);
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	if (dirty) {
		do_backing_store_page_out(location);
	}
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	key = k_spin_lock(&z_mm_lock);
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	z_mem_page_frame_free_locked(pf);
out:
	k_spin_unlock(&z_mm_lock, key);
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
#ifdef CONFIG_SMP
	k_mutex_unlock(&z_mm_paging_lock);
#else
	k_sched_unlock();
#endif
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	return ret;
}

static inline void paging_stats_faults_inc(struct k_thread *faulting_thread,
					   int key)
{
#ifdef CONFIG_DEMAND_PAGING_STATS
	bool is_irq_unlocked = arch_irq_unlocked(key);

	paging_stats.pagefaults.cnt++;

	if (is_irq_unlocked) {
		paging_stats.pagefaults.irq_unlocked++;
	} else {
		paging_stats.pagefaults.irq_locked++;
	}

#ifdef CONFIG_DEMAND_PAGING_THREAD_STATS
	faulting_thread->paging_stats.pagefaults.cnt++;

	if (is_irq_unlocked) {
		faulting_thread->paging_stats.pagefaults.irq_unlocked++;
	} else {
		faulting_thread->paging_stats.pagefaults.irq_locked++;
	}
#else
	ARG_UNUSED(faulting_thread);
#endif /* CONFIG_DEMAND_PAGING_THREAD_STATS */

#ifndef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	if (k_is_in_isr()) {
		paging_stats.pagefaults.in_isr++;

#ifdef CONFIG_DEMAND_PAGING_THREAD_STATS
		faulting_thread->paging_stats.pagefaults.in_isr++;
#endif /* CONFIG_DEMAND_PAGING_THREAD_STATS */
	}
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
#endif /* CONFIG_DEMAND_PAGING_STATS */
}

static inline void paging_stats_eviction_inc(struct k_thread *faulting_thread,
					     bool dirty)
{
#ifdef CONFIG_DEMAND_PAGING_STATS
	if (dirty) {
		paging_stats.eviction.dirty++;
	} else {
		paging_stats.eviction.clean++;
	}
#ifdef CONFIG_DEMAND_PAGING_THREAD_STATS
	if (dirty) {
		faulting_thread->paging_stats.eviction.dirty++;
	} else {
		faulting_thread->paging_stats.eviction.clean++;
	}
#else
	ARG_UNUSED(faulting_thread);
#endif /* CONFIG_DEMAND_PAGING_THREAD_STATS */
#endif /* CONFIG_DEMAND_PAGING_STATS */
}

static inline struct k_mem_page_frame *do_eviction_select(bool *dirty)
{
	struct k_mem_page_frame *pf;

#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
	uint32_t time_diff;

#ifdef CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS
	timing_t time_start, time_end;

	time_start = timing_counter_get();
#else
	uint32_t time_start;

	time_start = k_cycle_get_32();
#endif /* CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS */
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */

	pf = k_mem_paging_eviction_select(dirty);

#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
#ifdef CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS
	time_end = timing_counter_get();
	time_diff = (uint32_t)timing_cycles_get(&time_start, &time_end);
#else
	time_diff = k_cycle_get_32() - time_start;
#endif /* CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS */

	z_paging_histogram_inc(&z_paging_histogram_eviction, time_diff);
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */

	return pf;
}

static bool do_page_fault(void *addr, bool pin)
{
	struct k_mem_page_frame *pf;
	k_spinlock_key_t key;
	uintptr_t page_in_location, page_out_location;
	enum arch_page_location status;
	bool result;
	bool dirty = false;
	struct k_thread *faulting_thread;
	int ret;

	__ASSERT(z_mem_page_frames_initialized, "page fault at %p happened too early",
		 addr);

	LOG_DBG("page fault at %p", addr);

	/*
	 * TODO: Add performance accounting:
	 * - k_mem_paging_eviction_select() metrics
	 *   * periodic timer execution time histogram (if implemented)
	 */

#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	/*
	 * We do re-enable interrupts during the page-in/page-out operation
	 * if and only if interrupts were enabled when the exception was
	 * taken; in this configuration page faults in an ISR are a bug; all
	 * their code/data must be pinned.
	 *
	 * If interrupts were disabled when the exception was taken, the
	 * arch code is responsible for keeping them that way when entering
	 * this function.
	 *
	 * If this is not enabled, then interrupts are always locked for the
	 * entire operation. This is far worse for system interrupt latency
	 * but requires less pinned pages and ISRs may also take page faults.
	 *
	 * On UP we lock the scheduler so that other threads are never
	 * scheduled during the page-in/out operation. Support for
	 * allowing k_mem_paging_backing_store_page_out() and
	 * k_mem_paging_backing_store_page_in() to also sleep and allow
	 * other threads to run (such as in the case where the transfer is
	 * async DMA) is not supported on UP. Even if limited to thread
	 * context, arbitrary memory access triggering exceptions that put
	 * a thread to sleep on a contended page fault operation will break
	 * scheduling assumptions of cooperative threads or threads that
	 * implement critical sections with spinlocks or disabling IRQs.
	 *
	 * On SMP, though, exclusivity cannot be assumed solely from being
	 * a cooperative thread. Another thread with any prio may be running
	 * on another CPU so exclusion must already be enforced by other
	 * means. Therefore trying to prevent scheduling on SMP is pointless,
	 * and k_sched_lock()  is equivalent to a no-op on SMP anyway.
	 * As a result, sleeping/rescheduling in the SMP case is fine.
	 */
	__ASSERT(!k_is_in_isr(), "ISR page faults are forbidden");
#ifdef CONFIG_SMP
	k_mutex_lock(&z_mm_paging_lock, K_FOREVER);
#else
	k_sched_lock();
#endif
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */

	key = k_spin_lock(&z_mm_lock);
	faulting_thread = _current;

	status = arch_page_location_get(addr, &page_in_location);
	if (status == ARCH_PAGE_LOCATION_BAD) {
		/* Return false to treat as a fatal error */
		result = false;
		goto out;
	}
	result = true;

	if (status == ARCH_PAGE_LOCATION_PAGED_IN) {
		if (pin) {
			/* It's a physical memory address */
			uintptr_t phys = page_in_location;

			pf = k_mem_phys_to_page_frame(phys);
			if (!k_mem_page_frame_is_pinned(pf)) {
				if (IS_ENABLED(CONFIG_EVICTION_TRACKING)) {
					k_mem_paging_eviction_remove(pf);
				}
				k_mem_page_frame_set(pf, K_MEM_PAGE_FRAME_PINNED);
			}
		}

		/* This if-block is to pin the page if it is
		 * already present in physical memory. There is
		 * no need to go through the following code to
		 * pull in the data pages. So skip to the end.
		 */
		goto out;
	}
	__ASSERT(status == ARCH_PAGE_LOCATION_PAGED_OUT,
		 "unexpected status value %d", status);

	paging_stats_faults_inc(faulting_thread, key.key);

	pf = z_mem_page_frame_get_free();
	if (pf == NULL) {
		/* Need to evict a page frame */
		pf = do_eviction_select(&dirty);
		if (pf == NULL) {
			/* Every evictable page frame is pinned or busy.
			 * Fail the fault so it is reported as fatal.
			 */
			result = false;
			goto out;
		}
		LOG_DBG("evicting %p at 0x%lx",
			k_mem_page_frame_to_virt(pf),
			k_mem_page_frame_to_phys(pf));

		paging_stats_eviction_inc(faulting_thread, dirty);
	}
	ret = page_frame_prepare_locked(pf, &dirty, true, &page_out_location);
	if (ret != 0) {
		/* Backing store is full. Fail the fault so it is
		 * reported as fatal.
		 */
		result = false;
		goto out;
	}

#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	k_spin_unlock(&z_mm_lock, key);
	/* Interrupts are now unlocked if they were not locked when we entered
	 * this function, and we may service ISRs. The scheduler is still
	 * locked.
	 */
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	if (dirty) {
		do_backing_store_page_out(page_out_location);
	}
	do_backing_store_page_in(page_in_location);

#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
	key = k_spin_lock(&z_mm_lock);
	k_mem_page_frame_clear(pf, K_MEM_PAGE_FRAME_BUSY);
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */
	k_mem_page_frame_clear(pf, K_MEM_PAGE_FRAME_MAPPED);
	z_mem_page_frame_mapped_set(pf, addr);
	if (pin) {
		k_mem_page_frame_set(pf, K_MEM_PAGE_FRAME_PINNED);
	}

	arch_mem_page_in(addr, k_mem_page_frame_to_phys(pf));
	k_mem_paging_backing_store_page_finalize(pf, page_in_location);
	if (IS_ENABLED(CONFIG_EVICTION_TRACKING) && (!pin)) {
		k_mem_paging_eviction_add(pf);
	}
out:
	k_spin_unlock(&z_mm_lock, key);
#ifdef CONFIG_DEMAND_PAGING_ALLOW_IRQ
#ifdef CONFIG_SMP
	k_mutex_unlock(&z_mm_paging_lock);
#else
	k_sched_unlock();
#endif
#endif /* CONFIG_DEMAND_PAGING_ALLOW_IRQ */

	return result;
}

static void do_page_in(void *addr)
{
	bool ret;

	ret = do_page_fault(addr, false);
	if (!ret) {
		/* There is no error reporting channel to the caller, and
		 * continuing would defer the failure to an arbitrary later
		 * access.
		 */
		LOG_ERR("cannot page in address %p", addr);
		k_panic();
	}
}

void k_mem_page_in(void *addr, size_t size)
{
	__ASSERT(!IS_ENABLED(CONFIG_DEMAND_PAGING_ALLOW_IRQ) || !k_is_in_isr(),
		 "%s may not be called in ISRs if CONFIG_DEMAND_PAGING_ALLOW_IRQ is enabled",
		 __func__);
	virt_region_foreach(addr, size, do_page_in);
}

static void do_mem_pin(void *addr)
{
	bool ret;

	ret = do_page_fault(addr, true);
	if (!ret) {
		/* There is no error reporting channel to the caller, and
		 * continuing with the memory not actually pinned would
		 * defer the failure to an arbitrary later access, possibly
		 * from a context that cannot handle a page fault.
		 */
		LOG_ERR("cannot pin address %p", addr);
		k_panic();
	}
}

void k_mem_pin(void *addr, size_t size)
{
	__ASSERT(!IS_ENABLED(CONFIG_DEMAND_PAGING_ALLOW_IRQ) || !k_is_in_isr(),
		 "%s may not be called in ISRs if CONFIG_DEMAND_PAGING_ALLOW_IRQ is enabled",
		 __func__);
	virt_region_foreach(addr, size, do_mem_pin);
}

bool k_mem_page_fault(void *addr)
{
	return do_page_fault(addr, false);
}

static void do_mem_unpin(void *addr)
{
	struct k_mem_page_frame *pf;
	k_spinlock_key_t key;
	uintptr_t flags, phys;

	key = k_spin_lock(&z_mm_lock);
	flags = arch_page_info_get(addr, &phys, false);
	__ASSERT((flags & ARCH_DATA_PAGE_NOT_MAPPED) == 0,
		 "invalid data page at %p", addr);
	if ((flags & ARCH_DATA_PAGE_LOADED) != 0) {
		pf = k_mem_phys_to_page_frame(phys);
		if (k_mem_page_frame_is_pinned(pf)) {
			k_mem_page_frame_clear(pf, K_MEM_PAGE_FRAME_PINNED);

			if (IS_ENABLED(CONFIG_EVICTION_TRACKING)) {
				k_mem_paging_eviction_add(pf);
			}
		}
	}
	k_spin_unlock(&z_mm_lock, key);
}

void k_mem_unpin(void *addr, size_t size)
{
	__ASSERT(z_mem_page_frames_initialized, "%s called on %p too early", __func__,
		 addr);
	virt_region_foreach(addr, size, do_mem_unpin);
}
