/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <syscall_handler.h>
#include <toolchain.h>
#include <sys/mem_manage.h>

extern struct k_mem_paging_stats_t paging_stats;

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
void z_impl_k_mem_paging_thread_stats_get(k_tid_t tid,
					  struct k_mem_paging_stats_t *stats)
{
	if ((tid == NULL) || (stats == NULL)) {
		return;
	}

	/* Copy statistics */
	memcpy(stats, &tid->paging_stats, sizeof(tid->paging_stats));
}

#ifdef CONFIG_USERSPACE
static inline
void z_vrfy_k_mem_paging_thread_stats_get(k_tid_t tid,
					  struct k_mem_paging_stats_t *stats)
{
	Z_OOPS(Z_SYSCALL_OBJ(tid, K_OBJ_THREAD));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(stats, sizeof(*stats)));
	z_impl_k_mem_paging_thread_stats_get(tid, stats);
}
#include <syscalls/k_mem_paging_thread_stats_get_mrsh.c>
#endif /* CONFIG_USERSPACE */

#endif /* CONFIG_DEMAND_PAGING_THREAD_STATS */
