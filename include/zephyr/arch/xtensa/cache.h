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

#ifdef __cplusplus
extern "C" {
#endif

#define Z_DCACHE_MAX (XCHAL_DCACHE_SIZE / XCHAL_DCACHE_WAYS)

#if XCHAL_DCACHE_SIZE
BUILD_ASSERT(Z_IS_POW2(XCHAL_DCACHE_LINESIZE));
BUILD_ASSERT(Z_IS_POW2(Z_DCACHE_MAX));
#endif

#if defined(CONFIG_DCACHE)
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

static ALWAYS_INLINE void arch_dcache_enable(void)
{
	/* nothing */
}

static ALWAYS_INLINE void arch_dcache_disable(void)
{
	/* nothing */
}

#endif /* CONFIG_DCACHE */

#if defined(CONFIG_ICACHE)

static size_t arch_icache_line_size_get(void)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_icache_invd_all(void)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_icache_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_icache_flush_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_icache_invd_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE int arch_icache_flush_and_invd_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

static ALWAYS_INLINE void arch_icache_enable(void)
{
	/* nothing */
}

static ALWAYS_INLINE vid arch_icache_disable(void)
{
	/* nothing */
}

#endif /* CONFIG_ICACHE */



#if defined(CONFIG_XTENSA_RPO_CACHE)
#if defined(CONFIG_ARCH_HAS_COHERENCE)
static inline bool arch_mem_coherent(void *ptr)
{
	size_t addr = (size_t) ptr;

	return (addr >> 29) == CONFIG_XTENSA_UNCACHED_REGION;
}
#endif

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
 * @see arch_xtensa_uncached_ptr()
 *
 * @param ptr A pointer to a valid C object
 * @return A pointer to the same object via the L1 dcache
 */
static inline void __sparse_cache *arch_xtensa_cached_ptr(void *ptr)
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
 * @see arch_xtensa_cached_ptr()
 *
 * @param ptr A pointer to a valid C object
 * @return A pointer to the same object bypassing the L1 dcache
 */
static inline void *arch_xtensa_uncached_ptr(void __sparse_cache *ptr)
{
	return (void *)z_xtrpoflip((__sparse_force uint32_t)ptr,
				   CONFIG_XTENSA_UNCACHED_REGION,
				   CONFIG_XTENSA_CACHED_REGION);
}

/* Utility to generate an unrolled and optimal[1] code sequence to set
 * the RPO TLB registers (contra the HAL cacheattr macros, which
 * generate larger code and can't be called from C), based on the
 * KERNEL_COHERENCE configuration in use.  Selects RPO attribute "2"
 * for regions (including MMIO registers in region zero) which want to
 * bypass L1, "4" for the cached region which wants writeback, and
 * "15" (invalid) elsewhere.
 *
 * Note that on cores that have the "translation" option set, we need
 * to put an identity mapping in the high bits.  Also per spec
 * changing the current code region (by definition cached) requires
 * that WITLB be followed by an ISYNC and that both instructions live
 * in the same cache line (two 3-byte instructions fit in an 8-byte
 * aligned region, so that's guaranteed not to cross a cache line
 * boundary).
 *
 * [1] With the sole exception of gcc's infuriating insistence on
 * emitting a precomputed literal for addr + addrincr instead of
 * computing it with a single ADD instruction from values it already
 * has in registers.  Explicitly assigning the variables to registers
 * via an attribute works, but then emits needless MOV instructions
 * instead.  I tell myself it's just 32 bytes of .text, but... Sigh.
 */
#define _REGION_ATTR(r)						\
	((r) == 0 ? 2 :						\
	 ((r) == CONFIG_XTENSA_CACHED_REGION ? 4 :		\
	  ((r) == CONFIG_XTENSA_UNCACHED_REGION ? 2 : 15)))

#define _SET_ONE_TLB(region) do {				\
	uint32_t attr = _REGION_ATTR(region);			\
	if (XCHAL_HAVE_XLT_CACHEATTR) {				\
		attr |= addr; /* RPO with translation */	\
	}							\
	if (region != CONFIG_XTENSA_CACHED_REGION) {		\
		__asm__ volatile("wdtlb %0, %1; witlb %0, %1"	\
				 :: "r"(attr), "r"(addr));	\
	} else {						\
		__asm__ volatile("wdtlb %0, %1"			\
				 :: "r"(attr), "r"(addr));	\
		__asm__ volatile("j 1f; .align 8; 1:");		\
		__asm__ volatile("witlb %0, %1; isync"		\
				 :: "r"(attr), "r"(addr));	\
	}							\
	addr += addrincr;					\
} while (0)

#define ARCH_XTENSA_SET_RPO_TLB() do {				\
	register uint32_t addr = 0, addrincr = 0x20000000;	\
	FOR_EACH(_SET_ONE_TLB, (;), 0, 1, 2, 3, 4, 5, 6, 7);	\
} while (0)

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_CACHE_H_ */
