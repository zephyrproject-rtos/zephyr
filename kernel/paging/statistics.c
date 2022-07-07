/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/mem_manage.h>

extern struct k_mem_paging_stats_t paging_stats;

#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
struct k_mem_paging_histogram_t z_paging_histogram_eviction;
struct k_mem_paging_histogram_t z_paging_histogram_backing_store_page_in;
struct k_mem_paging_histogram_t z_paging_histogram_backing_store_page_out;

#ifdef CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS

/*
 * The frequency of timing functions is highly dependent on
 * architecture, SoC or board. It is also not available at build time.
 * Therefore, the bounds for the timing histograms needs to be defined
 * externally to this file, and must be tailored to the platform
 * being used.
 */

extern unsigned long
k_mem_paging_eviction_histogram_bounds[
	CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS];

extern unsigned long
k_mem_paging_backing_store_histogram_bounds[
	CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS];

#else
#define NS_TO_CYC(ns)		(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000000U * ns)

/*
 * This provides the upper bounds of the bins in eviction timing histogram.
 */
__weak unsigned long
k_mem_paging_eviction_histogram_bounds[CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS] = {
	NS_TO_CYC(1),
	NS_TO_CYC(5),
	NS_TO_CYC(10),
	NS_TO_CYC(50),
	NS_TO_CYC(100),
	NS_TO_CYC(200),
	NS_TO_CYC(500),
	NS_TO_CYC(1000),
	NS_TO_CYC(2000),
	ULONG_MAX
};

/*
 * This provides the upper bounds of the bins in backing store timing histogram
 * (both page-in and page-out).
 */
__weak unsigned long
k_mem_paging_backing_store_histogram_bounds[
	CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS] = {
	NS_TO_CYC(10),
	NS_TO_CYC(100),
	NS_TO_CYC(125),
	NS_TO_CYC(250),
	NS_TO_CYC(500),
	NS_TO_CYC(1000),
	NS_TO_CYC(2000),
	NS_TO_CYC(5000),
	NS_TO_CYC(10000),
	ULONG_MAX
};
#endif /* CONFIG_DEMAND_PAGING_STATS_USING_TIMING_FUNCTIONS */
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */

unsigned long z_num_pagefaults_get(void)
{
	unsigned long ret;
	int key;

	key = irq_lock();
	ret = paging_stats.pagefaults.cnt;
	irq_unlock(key);

	return ret;
}

void z_impl_k_mem_paging_stats_get(struct k_mem_paging_stats_t *stats)
{
	if (stats == NULL) {
		return;
	}

	/* Copy statistics */
	memcpy(stats, &paging_stats, sizeof(paging_stats));
}

#ifdef CONFIG_USERSPACE
static inline
void z_vrfy_k_mem_paging_stats_get(struct k_mem_paging_stats_t *stats)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(stats, sizeof(*stats)));
	z_impl_k_mem_paging_stats_get(stats);
}
#include <syscalls/k_mem_paging_stats_get_mrsh.c>
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_DEMAND_PAGING_THREAD_STATS
void z_impl_k_mem_paging_thread_stats_get(struct k_thread *thread,
					  struct k_mem_paging_stats_t *stats)
{
	if ((thread == NULL) || (stats == NULL)) {
		return;
	}

	/* Copy statistics */
	memcpy(stats, &thread->paging_stats, sizeof(thread->paging_stats));
}

#ifdef CONFIG_USERSPACE
static inline
void z_vrfy_k_mem_paging_thread_stats_get(struct k_thread *thread,
					  struct k_mem_paging_stats_t *stats)
{
	Z_OOPS(Z_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(stats, sizeof(*stats)));
	z_impl_k_mem_paging_thread_stats_get(thread, stats);
}
#include <syscalls/k_mem_paging_thread_stats_get_mrsh.c>
#endif /* CONFIG_USERSPACE */

#endif /* CONFIG_DEMAND_PAGING_THREAD_STATS */

#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
void z_paging_histogram_init(void)
{
	/*
	 * Zero out the histogram structs and copy the bounds.
	 * The copying is done as the histogram structs need
	 * to be pinned in memory and never swapped out, while
	 * the source bound array may not be pinned.
	 */

	memset(&z_paging_histogram_eviction, 0, sizeof(z_paging_histogram_eviction));
	memcpy(z_paging_histogram_eviction.bounds,
	       k_mem_paging_eviction_histogram_bounds,
	       sizeof(z_paging_histogram_eviction.bounds));

	memset(&z_paging_histogram_backing_store_page_in, 0,
	       sizeof(z_paging_histogram_backing_store_page_in));
	memcpy(z_paging_histogram_backing_store_page_in.bounds,
	       k_mem_paging_backing_store_histogram_bounds,
	       sizeof(z_paging_histogram_backing_store_page_in.bounds));

	memset(&z_paging_histogram_backing_store_page_out, 0,
	       sizeof(z_paging_histogram_backing_store_page_out));
	memcpy(z_paging_histogram_backing_store_page_out.bounds,
	       k_mem_paging_backing_store_histogram_bounds,
	       sizeof(z_paging_histogram_backing_store_page_out.bounds));
}

/**
 * Increment the counter in the timing histogram.
 *
 * @param hist The timing histogram to be updated.
 * @param cycles Time spent in measured operation.
 */
void z_paging_histogram_inc(struct k_mem_paging_histogram_t *hist,
			    uint32_t cycles)
{
	int idx;

	for (idx = 0;
	     idx < CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM_NUM_BINS;
	     idx++) {
		if (cycles <= hist->bounds[idx]) {
			hist->counts[idx]++;
			break;
		}
	}
}

void z_impl_k_mem_paging_histogram_eviction_get(
	struct k_mem_paging_histogram_t *hist)
{
	if (hist == NULL) {
		return;
	}

	/* Copy statistics */
	memcpy(hist, &z_paging_histogram_eviction,
	       sizeof(z_paging_histogram_eviction));
}

void z_impl_k_mem_paging_histogram_backing_store_page_in_get(
	struct k_mem_paging_histogram_t *hist)
{
	if (hist == NULL) {
		return;
	}

	/* Copy histogram */
	memcpy(hist, &z_paging_histogram_backing_store_page_in,
	       sizeof(z_paging_histogram_backing_store_page_in));
}

void z_impl_k_mem_paging_histogram_backing_store_page_out_get(
	struct k_mem_paging_histogram_t *hist)
{
	if (hist == NULL) {
		return;
	}

	/* Copy histogram */
	memcpy(hist, &z_paging_histogram_backing_store_page_out,
	       sizeof(z_paging_histogram_backing_store_page_out));
}

#ifdef CONFIG_USERSPACE
static inline
void z_vrfy_k_mem_paging_histogram_eviction_get(
	struct k_mem_paging_histogram_t *hist)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(hist, sizeof(*hist)));
	z_impl_k_mem_paging_histogram_eviction_get(hist);
}
#include <syscalls/k_mem_paging_histogram_eviction_get_mrsh.c>

static inline
void z_vrfy_k_mem_paging_histogram_backing_store_page_in_get(
	struct k_mem_paging_histogram_t *hist)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(hist, sizeof(*hist)));
	z_impl_k_mem_paging_histogram_backing_store_page_in_get(hist);
}
#include <syscalls/k_mem_paging_histogram_backing_store_page_in_get_mrsh.c>

static inline
void z_vrfy_k_mem_paging_histogram_backing_store_page_out_get(
	struct k_mem_paging_histogram_t *hist)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(hist, sizeof(*hist)));
	z_impl_k_mem_paging_histogram_backing_store_page_out_get(hist);
}
#include <syscalls/k_mem_paging_histogram_backing_store_page_out_get_mrsh.c>
#endif /* CONFIG_USERSPACE */

#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */
