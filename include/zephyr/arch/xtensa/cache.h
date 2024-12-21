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

#if defined(CONFIG_CACHE_DOUBLEMAP)
/**
 * @brief Test if a pointer is in cached region.
 *
 * Some hardware may map the same physical memory twice
 * so that it can be seen in both (incoherent) cached mappings
 * and a coherent "shared" area. This tests if a particular
 * pointer is within the cached, coherent area.
 *
 * @param ptr Pointer
 *
 * @retval True if pointer is in cached region.
 * @retval False if pointer is not in cached region.
 */
static inline bool arch_cache_is_ptr_cached(void *ptr)
{
	size_t addr = (size_t) ptr;

	return (addr >> 29) == CONFIG_XTENSA_CACHED_REGION;
}

/**
 * @brief Test if a pointer is in un-cached region.
 *
 * Some hardware may map the same physical memory twice
 * so that it can be seen in both (incoherent) cached mappings
 * and a coherent "shared" area. This tests if a particular
 * pointer is within the un-cached, incoherent area.
 *
 * @param ptr Pointer
 *
 * @retval True if pointer is not in cached region.
 * @retval False if pointer is in cached region.
 */
static inline bool arch_cache_is_ptr_uncached(void *ptr)
{
	size_t addr = (size_t) ptr;

	return (addr >> 29) == CONFIG_XTENSA_UNCACHED_REGION;
}

static ALWAYS_INLINE uint32_t z_xtrpoflip(uint32_t addr, uint32_t rto, uint32_t rfrom)
{
	/* The math here is all compile-time: when the two regions
	 * differ by a power of two, we can convert between them by
	 * setting or clearing just one bit.  Otherwise it needs two
	 * operations.
	 */
	uint32_t rxor = (rto ^ rfrom) << 29;

	rto <<= 29;
	if (Z_IS_POW2(rxor)) {
		if ((rxor & rto) == 0) {
			return addr & ~rxor;
		} else {
			return addr | rxor;
		}
	} else {
		return (addr & ~(7U << 29)) | rto;
	}
}

/**
 * @brief Return cached pointer to a RAM address
 *
 * The Xtensa coherence architecture maps addressable RAM twice, in
 * two different 512MB regions whose L1 cache settings can be
 * controlled independently.  So for any given pointer, it is possible
 * to convert it to and from a cached version.
 *
 * This function takes a pointer to any addressable object (either in
 * cacheable memory or not) and returns a pointer that can be used to
 * refer to the same memory through the L1 data cache.  Data read
 * through the resulting pointer will reflect locally cached values on
 * the current CPU if they exist, and writes will go first into the
 * cache and be written back later.
 *
 * @see arch_uncached_ptr()
 *
 * @param ptr A pointer to a valid C object
 * @return A pointer to the same object via the L1 dcache
 */
static inline void __sparse_cache *arch_cache_cached_ptr_get(void *ptr)
{
	return (__sparse_force void __sparse_cache *)z_xtrpoflip((uint32_t) ptr,
						CONFIG_XTENSA_CACHED_REGION,
						CONFIG_XTENSA_UNCACHED_REGION);
}

/**
 * @brief Return uncached pointer to a RAM address
 *
 * The Xtensa coherence architecture maps addressable RAM twice, in
 * two different 512MB regions whose L1 cache settings can be
 * controlled independently.  So for any given pointer, it is possible
 * to convert it to and from a cached version.
 *
 * This function takes a pointer to any addressable object (either in
 * cacheable memory or not) and returns a pointer that can be used to
 * refer to the same memory while bypassing the L1 data cache.  Data
 * in the L1 cache will not be inspected nor modified by the access.
 *
 * @see arch_cached_ptr()
 *
 * @param ptr A pointer to a valid C object
 * @return A pointer to the same object bypassing the L1 dcache
 */
static inline void *arch_cache_uncached_ptr_get(void __sparse_cache *ptr)
{
	return (void *)z_xtrpoflip((__sparse_force uint32_t)ptr,
				   CONFIG_XTENSA_UNCACHED_REGION,
				   CONFIG_XTENSA_CACHED_REGION);
}
#else
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
#endif

static ALWAYS_INLINE void arch_cache_init(void)
{
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_CACHE_H_ */
