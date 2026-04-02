/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <errno.h>
#include <sys_malloc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sync_heap.h>

#define LOG_LEVEL CONFIG_KERNEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

Z_THREAD_LOCAL struct sys_sync_heap *malloc_heap;

#ifdef CONFIG_USERSPACE
static inline bool is_kernel_thread(void)
{
	if (arch_is_user_context()) {
		return false;
	}
	return (k_current_get()->base.user_options & K_USER) == 0;
}
#else
static inline bool is_kernel_thread(void)
{
	return true;
}
#endif /* CONFIG_USERSPACE */

static inline bool fallback_to_system_heap(void)
{
#ifdef CONFIG_COMMON_LIBC_MALLOC_TLS_FALLBACK
	return is_kernel_thread();
#else
	return false;
#endif /* CONFIG_COMMON_LIBC_MALLOC_TLS_FALLBACK */
}

void *aligned_alloc(size_t align, size_t size)
{
	void *ret = NULL;

	if (malloc_heap != NULL) {
		ret = sys_sync_heap_aligned_alloc(malloc_heap, align, size);
	} else if (fallback_to_system_heap()) {
		ret = k_aligned_alloc(align, size);
	}

	if (ret == NULL && size != 0) {
		errno = ENOMEM;
	}

	return ret;
}

void *malloc(size_t size)
{
	return aligned_alloc(__alignof__(z_max_align_t), size);
}

void *realloc(void *ptr, size_t requested_size)
{
	void *ret = NULL;

	if (malloc_heap != NULL) {
		ret = sys_sync_heap_aligned_realloc(malloc_heap, ptr, __alignof__(z_max_align_t),
						    requested_size);
	} else if (fallback_to_system_heap()) {
		ret = k_realloc(ptr, requested_size);
	}

	if (ret == NULL && requested_size != 0) {
		errno = ENOMEM;
	}

	return ret;
}

void free(void *ptr)
{
	if (malloc_heap != NULL) {
		sys_sync_heap_free(malloc_heap, ptr);
	} else if (fallback_to_system_heap()) {
		k_free(ptr);
	}
}

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
int malloc_runtime_stats_get(struct sys_memory_stats *stats)
{
	int ret = -ENOTSUP;

	if (malloc_heap != NULL) {
		ret = sys_sync_heap_runtime_stats_get(malloc_heap, stats);
	}

	return ret;
}
#endif /* CONFIG_SYS_HEAP_RUNTIME_STATS */
