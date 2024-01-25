/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xtensa specific kernel interface header
 * This header contains the Xtensa specific kernel interface.  It is included
 * by the generic kernel interface header (include/zephyr/arch/cpu.h)
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
#include <zephyr/arch/xtensa/syscall.h>
#include <zephyr/arch/xtensa/thread.h>
#include <zephyr/arch/xtensa/irq.h>
#include <xtensa/config/core.h>
#include <zephyr/arch/common/addr_types.h>
#include <zephyr/arch/xtensa/gdbstub.h>
#include <zephyr/debug/sparse.h>
#include <zephyr/arch/xtensa/thread_stack.h>
#include <zephyr/sys/slist.h>

#include <zephyr/drivers/timer/system_timer.h>

#include <zephyr/arch/xtensa/xtensa_mmu.h>

/**
 * @defgroup xtensa_apis Xtensa APIs
 * @{
 * @}
 *
 * @defgroup xtensa_internal_apis Xtensa Internal APIs
 * @ingroup xtensa_apis
 * @{
 * @}
 */

#include <zephyr/arch/xtensa/exception.h>

#ifdef __cplusplus
extern "C" {
#endif

struct arch_mem_domain {
#ifdef CONFIG_XTENSA_MMU
	uint32_t *ptables __aligned(CONFIG_MMU_PAGE_SIZE);
	uint8_t asid;
	bool dirty;
#endif
	sys_snode_t node;
};

/**
 * @brief Generate hardware exception.
 *
 * This generates hardware exception which is used by ARCH_EXCEPT().
 *
 * @param reason_p Reason for exception.
 */
extern void xtensa_arch_except(int reason_p);

/**
 * @brief Generate kernel oops.
 *
 * This generates kernel oops which is used by arch_syscall_oops().
 *
 * @param reason_p Reason for exception.
 * @param ssf Stack pointer.
 */
extern void xtensa_arch_kernel_oops(int reason_p, void *ssf);

#ifdef CONFIG_USERSPACE

#define ARCH_EXCEPT(reason_p) do { \
	if (k_is_user_context()) { \
		arch_syscall_invoke1(reason_p, \
			K_SYSCALL_XTENSA_USER_FAULT); \
	} else { \
		xtensa_arch_except(reason_p); \
	} \
	CODE_UNREACHABLE; \
} while (false)

#else

#define ARCH_EXCEPT(reason_p) do { \
		xtensa_arch_except(reason_p); \
		CODE_UNREACHABLE; \
	} while (false)

#endif

__syscall void xtensa_user_fault(unsigned int reason);

#include <syscalls/arch.h>

/* internal routine documented in C file, needed by IRQ_CONNECT() macro */
extern void z_irq_priority_set(uint32_t irq, uint32_t prio, uint32_t flags);

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
	{ \
		Z_ISR_DECLARE(irq_p, flags_p, isr_p, isr_param_p); \
	}

/** Implementation of @ref arch_k_cycle_get_32. */
static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

/** Implementation of @ref arch_k_cycle_get_64. */
static inline uint64_t arch_k_cycle_get_64(void)
{
	return sys_clock_cycle_get_64();
}

/** Implementation of @ref arch_nop. */
static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

/**
 * @brief Lock VECBASE if supported by hardware.
 *
 * The bit 0 of VECBASE acts as a lock bit on hardware supporting
 * this feature. When this bit is set, VECBASE cannot be changed
 * until it is cleared by hardware reset. When the hardware does not
 * support this bit, it is hardwired to 0.
 */
static ALWAYS_INLINE void xtensa_vecbase_lock(void)
{
	int vecbase;

	__asm__ volatile("rsr.vecbase %0" : "=r" (vecbase));
	__asm__ volatile("wsr.vecbase %0; rsync" : : "r" (vecbase | 1));
}

#if defined(CONFIG_XTENSA_RPO_CACHE) || defined(__DOXYGEN__)
#if defined(CONFIG_ARCH_HAS_COHERENCE) || defined(__DOXYGEN__)
/** Implementation of @ref arch_mem_coherent. */
static inline bool arch_mem_coherent(void *ptr)
{
	size_t addr = (size_t) ptr;

	return (addr >> 29) == CONFIG_XTENSA_UNCACHED_REGION;
}
#endif


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

/**
 * @brief Setup RPO TLB registers.
 */
#define ARCH_XTENSA_SET_RPO_TLB()					\
	do {								\
		register uint32_t addr = 0, addrincr = 0x20000000;	\
		FOR_EACH(_SET_ONE_TLB, (;), 0, 1, 2, 3, 4, 5, 6, 7);	\
	} while (0)
#endif /* CONFIG_XTENSA_RPO_CACHE */

#if defined(CONFIG_XTENSA_MMU) || defined(__DOXYGEN__)
/**
 * @brief Peform additional steps after MMU initialization.
 *
 * This performs additional steps related to memory management
 * after the main MMU initialization code. This needs to defined
 * in the SoC layer. Default is do no nothing.
 *
 * @param is_core0 True if this is called while executing on
 *                 CPU core #0.
 */
extern void arch_xtensa_mmu_post_init(bool is_core0);
#endif

#ifdef __cplusplus
}
#endif

#endif /* !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)  */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_ARCH_H_ */
