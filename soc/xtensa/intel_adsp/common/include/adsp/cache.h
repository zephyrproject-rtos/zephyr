/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __COMMON_ADSP_CACHE_H__
#define __COMMON_ADSP_CACHE_H__

#include <arch/xtensa/cache.h>

/* macros for data cache operations */
#define SOC_DCACHE_FLUSH(addr, size)		\
	z_xtensa_cache_flush((addr), (size))
#define SOC_DCACHE_INVALIDATE(addr, size)	\
	z_xtensa_cache_inv((addr), (size))

/**
 * @brief Return uncached pointer to a RAM address
 *
 * The Intel ADSP architecture maps all addressable RAM (of all types)
 * twice, in two different 512MB segments regions whose L1 cache
 * settings can be controlled independently.  So for any given
 * pointer, it is possible to convert it to and from a cached version.
 *
 * This function takes a pointer to any addressible object (either in
 * cacheable memory or not) and returns a pointer that can be used to
 * refer to the same memory while bypassing the L1 data cache.  Data
 * in the L1 cache will not be inspected nor modified by the access.
 *
 * @see z_soc_cached_ptr()
 *
 * @param p A pointer to a valid C object
 * @return A pointer to the same object bypassing the L1 dcache
 */
static inline void *z_soc_uncached_ptr(void *p)
{
	return ((void *)(((size_t)p) & ~0x20000000));
}

/**
 * @brief Return cached pointer to a RAM address
 *
 * This function takes a pointer to any addressible object (either in
 * cacheable memory or not) and returns a pointer that can be used to
 * refer to the same memory through the L1 data cache.  Data read
 * through the resulting pointer will reflect locally cached values on
 * the current CPU if they exist, and writes will go first into the
 * cache and be written back later.
 *
 * @see z_soc_uncached_ptr()
 *
 * @param p A pointer to a valid C object
 * @return A pointer to the same object via the L1 dcache

 */
static inline void *z_soc_cached_ptr(void *p)
{
	return ((void *)(((size_t)p) | 0x20000000));
}

#endif
