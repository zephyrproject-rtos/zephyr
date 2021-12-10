/*
 * Copyright 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_CACHE_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_CACHE_H_

#include <xtensa/config/core-isa.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define Z_DCACHE_MAX (XCHAL_DCACHE_SIZE / XCHAL_DCACHE_WAYS)

#if XCHAL_DCACHE_SIZE
#define Z_IS_POW2(x) (((x) != 0) && (((x) & ((x)-1)) == 0))
BUILD_ASSERT(Z_IS_POW2(XCHAL_DCACHE_LINESIZE));
BUILD_ASSERT(Z_IS_POW2(Z_DCACHE_MAX));
#endif

static ALWAYS_INLINE void z_xtensa_cache_flush(void *addr, size_t bytes)
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
}

static ALWAYS_INLINE void z_xtensa_cache_flush_inv(void *addr, size_t bytes)
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
}

static ALWAYS_INLINE void z_xtensa_cache_inv(void *addr, size_t bytes)
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
}

static ALWAYS_INLINE void z_xtensa_cache_inv_all(void)
{
	z_xtensa_cache_inv(NULL, Z_DCACHE_MAX);
}

static ALWAYS_INLINE void z_xtensa_cache_flush_all(void)
{
	z_xtensa_cache_flush(NULL, Z_DCACHE_MAX);
}

static ALWAYS_INLINE void z_xtensa_cache_flush_inv_all(void)
{
	z_xtensa_cache_flush_inv(NULL, Z_DCACHE_MAX);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_CACHE_H_ */
