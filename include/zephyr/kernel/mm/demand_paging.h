/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_MM_DEMAND_PAGING_H
#define ZEPHYR_INCLUDE_KERNEL_MM_DEMAND_PAGING_H

#include <zephyr/kernel/mm.h>

#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

/**
 * @defgroup demand_paging Demand Paging
 * @ingroup kernel_memory_management
 */

/**
 * @defgroup mem-demand-paging Demand Paging APIs
 * @ingroup demand_paging
 * @{
 */

#ifndef _ASMLANGUAGE
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <zephyr/sys/__assert.h>

/**
 * Paging Statistics.
 */
struct k_mem_paging_stats_t {
#if defined(CONFIG_DEMAND_PAGING_STATS) || defined(__DOXYGEN__)
	struct {
		/** Number of page faults */
		unsigned long			cnt;

		/** Number of page faults with IRQ locked */
		unsigned long			irq_locked;

		/** Number of page faults with IRQ unlocked */
		unsigned long			irq_unlocked;

#if !defined(CONFIG_DEMAND_PAGING_ALLOW_IRQ) || defined(__DOXYGEN__)
		/** Number of page faults while in ISR */
		unsigned long			in_isr;
#endif
	} pagefaults;

	struct {
		/** Number of clean pages selected for eviction */
		unsigned long			clean;

		/** Number of dirty pages selected for eviction */
		unsigned long			dirty;
	} eviction;
#endif /* CONFIG_DEMAND_PAGING_STATS */
};

/**
 * Paging Statistics Histograms.
 */
struct k_mem_paging_histogram_t {
#if defined(CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM) || defined(__DOXYGEN__)
	/* Counts for each bin in timing histogram */
	unsigned long	counts[CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS];

	/* Bounds for the bins in timing histogram,
	 * excluding the first and last (hence, NUM_SLOTS - 1).
	 */
	unsigned long	bounds[CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS];
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Evict a page-aligned virtual memory region to the backing store
 *
 * Useful if it is known that a memory region will not be used for some time.
 * All the data pages within the specified region will be evicted to the
 * backing store if they weren't already, with their associated page frames
 * marked as available for mappings or page-ins.
 *
 * None of the associated page frames mapped to the provided region should
 * be pinned.
 *
 * Note that there are no guarantees how long these pages will be evicted,
 * they could take page faults immediately.
 *
 * If CONFIG_DEMAND_PAGING_ALLOW_IRQ is enabled, this function may not be
 * called by ISRs as the backing store may be in-use.
 *
 * @param addr Base page-aligned virtual address
 * @param size Page-aligned data region size
 * @retval 0 Success
 * @retval -ENOMEM Insufficient space in backing store to satisfy request.
 *         The region may be partially paged out.
 */
int k_mem_page_out(void *addr, size_t size);

/**
 * Load a virtual data region into memory
 *
 * After the function completes, all the page frames associated with this
 * function will be paged in. However, they are not guaranteed to stay there.
 * This is useful if the region is known to be used soon.
 *
 * If CONFIG_DEMAND_PAGING_ALLOW_IRQ is enabled, this function may not be
 * called by ISRs as the backing store may be in-use.
 *
 * @param addr Base page-aligned virtual address
 * @param size Page-aligned data region size
 */
void k_mem_page_in(void *addr, size_t size);

/**
 * Pin an aligned virtual data region, paging in as necessary
 *
 * After the function completes, all the page frames associated with this
 * region will be resident in memory and pinned such that they stay that way.
 * This is a stronger version of z_mem_page_in().
 *
 * If CONFIG_DEMAND_PAGING_ALLOW_IRQ is enabled, this function may not be
 * called by ISRs as the backing store may be in-use.
 *
 * @param addr Base page-aligned virtual address
 * @param size Page-aligned data region size
 */
void k_mem_pin(void *addr, size_t size);

/**
 * Un-pin an aligned virtual data region
 *
 * After the function completes, all the page frames associated with this
 * region will be no longer marked as pinned. This does not evict the region,
 * follow this with z_mem_page_out() if you need that.
 *
 * @param addr Base page-aligned virtual address
 * @param size Page-aligned data region size
 */
void k_mem_unpin(void *addr, size_t size);

/**
 * Get the paging statistics since system startup
 *
 * This populates the paging statistics struct being passed in
 * as argument.
 *
 * @param[in,out] stats Paging statistics struct to be filled.
 */
__syscall void k_mem_paging_stats_get(struct k_mem_paging_stats_t *stats);

struct k_thread;
/**
 * Get the paging statistics since system startup for a thread
 *
 * This populates the paging statistics struct being passed in
 * as argument for a particular thread.
 *
 * @param[in] thread Thread
 * @param[in,out] stats Paging statistics struct to be filled.
 */
__syscall
void k_mem_paging_thread_stats_get(struct k_thread *thread,
				   struct k_mem_paging_stats_t *stats);

/**
 * Get the eviction timing histogram
 *
 * This populates the timing histogram struct being passed in
 * as argument.
 *
 * @param[in,out] hist Timing histogram struct to be filled.
 */
__syscall void k_mem_paging_histogram_eviction_get(
	struct k_mem_paging_histogram_t *hist);

/**
 * Get the backing store page-in timing histogram
 *
 * This populates the timing histogram struct being passed in
 * as argument.
 *
 * @param[in,out] hist Timing histogram struct to be filled.
 */
__syscall void k_mem_paging_histogram_backing_store_page_in_get(
	struct k_mem_paging_histogram_t *hist);

/**
 * Get the backing store page-out timing histogram
 *
 * This populates the timing histogram struct being passed in
 * as argument.
 *
 * @param[in,out] hist Timing histogram struct to be filled.
 */
__syscall void k_mem_paging_histogram_backing_store_page_out_get(
	struct k_mem_paging_histogram_t *hist);

#include <syscalls/demand_paging.h>

/** @} */

/**
 * Eviction algorithm APIs
 *
 * @defgroup mem-demand-paging-eviction Eviction Algorithm APIs
 * @ingroup demand_paging
 * @{
 */

/**
 * Select a page frame for eviction
 *
 * The kernel will invoke this to choose a page frame to evict if there
 * are no free page frames.
 *
 * This function will never be called before the initial
 * k_mem_paging_eviction_init().
 *
 * This function is invoked with interrupts locked.
 *
 * @param [out] dirty Whether the page to evict is dirty
 * @return The page frame to evict
 */
struct z_page_frame *k_mem_paging_eviction_select(bool *dirty);

/**
 * Initialization function
 *
 * Called at POST_KERNEL to perform any necessary initialization tasks for the
 * eviction algorithm. k_mem_paging_eviction_select() is guaranteed to never be
 * called until this has returned, and this will only be called once.
 */
void k_mem_paging_eviction_init(void);

/** @} */

/**
 * Backing store APIs
 *
 * @defgroup mem-demand-paging-backing-store Backing Store APIs
 * @ingroup demand_paging
 * @{
 */

/**
 * Reserve or fetch a storage location for a data page loaded into a page frame
 *
 * The returned location token must be unique to the mapped virtual address.
 * This location will be used in the backing store to page out data page
 * contents for later retrieval. The location value must be page-aligned.
 *
 * This function may be called multiple times on the same data page. If its
 * page frame has its Z_PAGE_FRAME_BACKED bit set, it is expected to return
 * the previous backing store location for the data page containing a cached
 * clean copy. This clean copy may be updated on page-out, or used to
 * discard clean pages without needing to write out their contents.
 *
 * If the backing store is full, some other backing store location which caches
 * a loaded data page may be selected, in which case its associated page frame
 * will have the Z_PAGE_FRAME_BACKED bit cleared (as it is no longer cached).
 *
 * pf->addr will indicate the virtual address the page is currently mapped to.
 * Large, sparse backing stores which can contain the entire address space
 * may simply generate location tokens purely as a function of pf->addr with no
 * other management necessary.
 *
 * This function distinguishes whether it was called on behalf of a page
 * fault. A free backing store location must always be reserved in order for
 * page faults to succeed. If the page_fault parameter is not set, this
 * function should return -ENOMEM even if one location is available.
 *
 * This function is invoked with interrupts locked.
 *
 * @param pf Virtual address to obtain a storage location
 * @param [out] location storage location token
 * @param page_fault Whether this request was for a page fault
 * @return 0 Success
 * @return -ENOMEM Backing store is full
 */
int k_mem_paging_backing_store_location_get(struct z_page_frame *pf,
					    uintptr_t *location,
					    bool page_fault);

/**
 * Free a backing store location
 *
 * Any stored data may be discarded, and the location token associated with
 * this address may be re-used for some other data page.
 *
 * This function is invoked with interrupts locked.
 *
 * @param location Location token to free
 */
void k_mem_paging_backing_store_location_free(uintptr_t location);

/**
 * Copy a data page from Z_SCRATCH_PAGE to the specified location
 *
 * Immediately before this is called, Z_SCRATCH_PAGE will be mapped read-write
 * to the intended source page frame for the calling context.
 *
 * Calls to this and k_mem_paging_backing_store_page_in() will always be
 * serialized, but interrupts may be enabled.
 *
 * @param location Location token for the data page, for later retrieval
 */
void k_mem_paging_backing_store_page_out(uintptr_t location);

/**
 * Copy a data page from the provided location to Z_SCRATCH_PAGE.
 *
 * Immediately before this is called, Z_SCRATCH_PAGE will be mapped read-write
 * to the intended destination page frame for the calling context.
 *
 * Calls to this and k_mem_paging_backing_store_page_out() will always be
 * serialized, but interrupts may be enabled.
 *
 * @param location Location token for the data page
 */
void k_mem_paging_backing_store_page_in(uintptr_t location);

/**
 * Update internal accounting after a page-in
 *
 * This is invoked after k_mem_paging_backing_store_page_in() and interrupts
 * have been* re-locked, making it safe to access the z_page_frame data.
 * The location value will be the same passed to
 * k_mem_paging_backing_store_page_in().
 *
 * The primary use-case for this is to update custom fields for the backing
 * store in the page frame, to reflect where the data should be evicted to
 * if it is paged out again. This may be a no-op in some implementations.
 *
 * If the backing store caches paged-in data pages, this is the appropriate
 * time to set the Z_PAGE_FRAME_BACKED bit. The kernel only skips paging
 * out clean data pages if they are noted as clean in the page tables and the
 * Z_PAGE_FRAME_BACKED bit is set in their associated page frame.
 *
 * @param pf Page frame that was loaded in
 * @param location Location of where the loaded data page was retrieved
 */
void k_mem_paging_backing_store_page_finalize(struct z_page_frame *pf,
					      uintptr_t location);

/**
 * Backing store initialization function.
 *
 * The implementation may expect to receive page in/out calls as soon as this
 * returns, but not before that. Called at POST_KERNEL.
 *
 * This function is expected to do two things:
 * - Initialize any internal data structures and accounting for the backing
 *   store.
 * - If the backing store already contains all or some loaded kernel data pages
 *   at boot time, Z_PAGE_FRAME_BACKED should be appropriately set for their
 *   associated page frames, and any internal accounting set up appropriately.
 */
void k_mem_paging_backing_store_init(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_KERNEL_MM_DEMAND_PAGING_H */
