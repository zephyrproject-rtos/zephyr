/*
 * Copyright 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_CACHE_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_CACHE_H_

#include <xtensa/config/core-isa.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/debug/sparse.h>
#include <xtensa/hal.h>

#ifdef __cplusplus
extern "C" {
#endif

#define Z_DCACHE_MAX (XCHAL_DCACHE_SIZE / XCHAL_DCACHE_WAYS)

#if XCHAL_DCACHE_SIZE
BUILD_ASSERT(Z_IS_POW2(XCHAL_DCACHE_LINESIZE));
BUILD_ASSERT(Z_IS_POW2(Z_DCACHE_MAX));
#endif

#if defined(CONFIG_DCACHE) || defined(__DOXYGEN__)

/** Implementation of @ref arch_dcache_flush_range. */
static ALWAYS_INLINE int arch_dcache_flush_range(void *addr, size_t bytes)
{
#if XCHAL_DCACHE_SIZE
	size_t step = XCHAL_DCACHE_LINESIZE;
	size_t first = ROUND_DOWN(addr, step);
	size_t last = ROUND_UP(((long)addr) + bytes, step);
	size_t line;

	for (line = first; bytes && line < last; line += step) {
		__asm__ volatile("dhwb %0, 0" :: "r"(line));
	}
#endif
	return 0;
}

/** Implementation of @ref arch_dcache_flush_and_invd_range. */
static ALWAYS_INLINE int arch_dcache_flush_and_invd_range(void *addr, size_t bytes)
{
#if XCHAL_DCACHE_SIZE
	size_t step = XCHAL_DCACHE_LINESIZE;
	size_t first = ROUND_DOWN(addr, step);
	size_t last = ROUND_UP(((long)addr) + bytes, step);
	size_t line;

	for (line = first; bytes && line < last; line += step) {
		__asm__ volatile("dhwbi %0, 0" :: "r"(line));
	}
#endif
	return 0;
}

/** Implementation of @ref arch_dcache_invd_range. */
static ALWAYS_INLINE int arch_dcache_invd_range(void *addr, size_t bytes)
{
#if XCHAL_DCACHE_SIZE
	size_t step = XCHAL_DCACHE_LINESIZE;
	size_t first = ROUND_DOWN(addr, step);
	size_t last = ROUND_UP(((long)addr) + bytes, step);
	size_t line;

	for (line = first; bytes && line < last; line += step) {
		__asm__ volatile("dhi %0, 0" :: "r"(line));
	}
#endif
	return 0;
}

/** Implementation of @ref arch_dcache_invd_all. */
static ALWAYS_INLINE int arch_dcache_invd_all(void)
{
#if XCHAL_DCACHE_SIZE
	size_t step = XCHAL_DCACHE_LINESIZE;
	size_t line;

	for (line = 0; line < XCHAL_DCACHE_SIZE; line += step) {
		__asm__ volatile("dii %0, 0" :: "r"(line));
	}
#endif
	return 0;
}

/** Implementation of @ref arch_dcache_flush_all. */
static ALWAYS_INLINE int arch_dcache_flush_all(void)
{
#if XCHAL_DCACHE_SIZE
	size_t step = XCHAL_DCACHE_LINESIZE;
	size_t line;

	for (line = 0; line < XCHAL_DCACHE_SIZE; line += step) {
		__asm__ volatile("diwb %0, 0" :: "r"(line));
	}
#endif
	return 0;
}

/** Implementation of @ref arch_dcache_flush_and_invd_all. */
static ALWAYS_INLINE int arch_dcache_flush_and_invd_all(void)
{
#if XCHAL_DCACHE_SIZE
	size_t step = XCHAL_DCACHE_LINESIZE;
	size_t line;

	for (line = 0; line < XCHAL_DCACHE_SIZE; line += step) {
		__asm__ volatile("diwbi %0, 0" :: "r"(line));
	}
#endif
	return 0;
}

/** Implementation of @ref arch_dcache_enable. */
static ALWAYS_INLINE void arch_dcache_enable(void)
{
	/* nothing */
}

/** Implementation of @ref arch_dcache_disable. */
static ALWAYS_INLINE void arch_dcache_disable(void)
{
	/* nothing */
}

#endif /* CONFIG_DCACHE */

#if defined(CONFIG_ICACHE) || defined(__DOXYGEN__)

/** Implementation of @ref arch_icache_line_size_get. */
static ALWAYS_INLINE size_t arch_icache_line_size_get(void)
{
	return -ENOTSUP;
}

/** Implementation of @ref arch_icache_flush_all. */
static ALWAYS_INLINE int arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

/** Implementation of @ref arch_icache_invd_all. */
static ALWAYS_INLINE int arch_icache_invd_all(void)
{
#if XCHAL_ICACHE_SIZE
	xthal_icache_all_invalidate();
#endif
	return 0;
}

/** Implementation of @ref arch_icache_flush_and_invd_all. */
static ALWAYS_INLINE int arch_icache_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

/** Implementation of @ref arch_icache_flush_range. */
static ALWAYS_INLINE int arch_icache_flush_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

/** Implementation of @ref arch_icache_invd_range. */
static ALWAYS_INLINE int arch_icache_invd_range(void *addr, size_t size)
{
#if XCHAL_ICACHE_SIZE
	xthal_icache_region_invalidate(addr, size);
#endif
	return 0;
}

/** Implementation of @ref arch_icache_flush_and_invd_range. */
static ALWAYS_INLINE int arch_icache_flush_and_invd_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

/** Implementation of @ref arch_icache_enable. */
static ALWAYS_INLINE void arch_icache_enable(void)
{
	/* nothing */
}

/** Implementation of @ref arch_icache_disable. */
static ALWAYS_INLINE void arch_icache_disable(void)
{
	/* nothing */
}

#endif /* CONFIG_ICACHE */

static inline bool arch_cache_is_ptr_cached(void *ptr)
{
	ARG_UNUSED(ptr);

	return false;
}

static inline bool arch_cache_is_ptr_uncached(void *ptr)
{
	ARG_UNUSED(ptr);

	return false;
}

static inline void *arch_cache_cached_ptr_get(void *ptr)
{
	return ptr;
}

static inline void *arch_cache_uncached_ptr_get(void *ptr)
{
	return ptr;
}

static ALWAYS_INLINE void arch_cache_init(void)
{
}

#if defined(CONFIG_CACHE_CAN_SAY_MEM_COHERENCE)
static ALWAYS_INLINE bool arch_mem_coherent(void *ptr)
{
	ARG_UNUSED(ptr);
	return true;
}
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_CACHE_H_ */
