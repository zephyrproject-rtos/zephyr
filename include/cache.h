/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CACHE_H_
#define ZEPHYR_INCLUDE_CACHE_H_

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

void arch_dcache_flush(void *addr, size_t size);
void arch_dcache_invd(void *addr, size_t size);

/**
 *
 * @brief Flush d-cache lines to main memory
 *
 * No alignment is required for either addr or size, but since
 * sys_cache_flush() iterates on the d-cache lines, a d-cache line alignment for
 * both is optimal.
 *
 * The d-cache line size is specified either via the CONFIG_CACHE_LINE_SIZE
 * kconfig option or it is detected at runtime.
 *
 * @param addr the pointer to start the multi-line flush
 * @param size the number of bytes that are to be flushed
 *
 * @return N/A
 */
__syscall void sys_cache_flush(void *addr, size_t size);

static inline void z_impl_sys_cache_flush(void *addr, size_t size)
{
	if (IS_ENABLED(CONFIG_CACHE_FLUSHING)) {
		arch_dcache_flush(addr, size);
	}
}

/**
 *
 * @brief Invalidate d-cache lines
 *
 * No alignment is required for either addr or size, but since
 * sys_cache_invd() iterates on the d-cache lines, a d-cache line alignment for
 * both is optimal.
 *
 * The d-cache line size is specified either via the CONFIG_CACHE_LINE_SIZE
 * kconfig option or it is detected at runtime.
 *
 * @param addr the pointer to start address
 * @param size the number of bytes that are to be invalidated
 *
 * @return N/A
 */
__syscall void sys_cache_invd(void *addr, size_t size);

static inline void z_impl_sys_cache_invd(void *addr, size_t size)
{
	if (IS_ENABLED(CONFIG_CACHE_FLUSHING)) {
		arch_dcache_invd(addr, size);
	}
}

/**
 *
 * @brief Get the d-cache line size.
 *
 * The API is provided to get the cache line size.
 *
 * @return size of the cache line or 0 if the cache is not enabled.
 */
static inline size_t sys_cache_line_size_get(void)
{
#ifdef CONFIG_CACHE_FLUSHING
#ifdef CONFIG_CACHE_LINE_SIZE
	return CONFIG_CACHE_LINE_SIZE;
#else
	return arch_cache_line_size_get();
#endif /* CONFIG_CACHE_LINE_SIZE */
#else
	return 0;
#endif /* CONFIG_CACHE_FLUSHING */
}

#include <syscalls/cache.h>
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CACHE_H_ */
