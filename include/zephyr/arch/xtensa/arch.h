/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xtensa specific kernel interface header
 * This header contains the Xtensa specific kernel interface.  It is included
 * by the generic kernel interface header (include/arch/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_H_

#include <zephyr/irq.h>

#include <zephyr/devicetree.h>
#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
#include <zephyr/types.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/arch/xtensa/thread.h>
#include <zephyr/arch/xtensa/irq.h>
#include <xtensa/config/core.h>
#include <zephyr/arch/common/addr_types.h>
#include <zephyr/arch/xtensa/gdbstub.h>
#include <zephyr/debug/sparse.h>

#include <zephyr/arch/xtensa/xtensa_mmu.h>

#ifdef CONFIG_KERNEL_COHERENCE
#define ARCH_STACK_PTR_ALIGN XCHAL_DCACHE_LINESIZE
#else
#define ARCH_STACK_PTR_ALIGN 16
#endif

/* Xtensa GPRs are often designated by two different names */
#define sys_define_gpr_with_alias(name1, name2) union { uint32_t name1, name2; }

#include <zephyr/arch/xtensa/exc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void xtensa_arch_except(int reason_p);

#define ARCH_EXCEPT(reason_p) do { \
	xtensa_arch_except(reason_p); \
	CODE_UNREACHABLE; \
} while (false)

/* internal routine documented in C file, needed by IRQ_CONNECT() macro */
extern void z_irq_priority_set(uint32_t irq, uint32_t prio, uint32_t flags);

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
{ \
	Z_ISR_DECLARE(irq_p, flags_p, isr_p, isr_param_p); \
}

#define XTENSA_ERR_NORET

extern uint32_t sys_clock_cycle_get_32(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

extern uint64_t sys_clock_cycle_get_64(void);

static inline uint64_t arch_k_cycle_get_64(void)
{
	return sys_clock_cycle_get_64();
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

static ALWAYS_INLINE void xtensa_vecbase_lock(void)
{
	int vecbase;

	__asm__ volatile("rsr.vecbase %0" : "=r" (vecbase));

	/* In some targets the bit 0 of VECBASE works as lock bit.
	 * When this bit set, VECBASE can't be changed until it is cleared by
	 * reset. When the target does not have it, it is hardwired to 0.
	 **/
	__asm__ volatile("wsr.vecbase %0; rsync" : : "r" (vecbase | 1));
}

#if defined(CONFIG_XTENSA_RPO_CACHE)
#if defined(CONFIG_ARCH_HAS_COHERENCE)
static inline bool arch_mem_coherent(void *ptr)
{
	size_t addr = (size_t) ptr;

	return (addr >> 29) == CONFIG_XTENSA_UNCACHED_REGION;
}
#endif

static inline bool arch_xtensa_is_ptr_cached(void *ptr)
{
	size_t addr = (size_t) ptr;

	return (addr >> 29) == CONFIG_XTENSA_CACHED_REGION;
}

static inline bool arch_xtensa_is_ptr_uncached(void *ptr)
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

#ifdef CONFIG_XTENSA_MMU
extern void arch_xtensa_mmu_post_init(bool is_core0);
#endif

#ifdef __cplusplus
}
#endif

#endif /* !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)  */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_H_ */
