/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPARC specific kernel interface header
 *
 * This header contains the SPARC specific kernel interface.  It is
 * included by the kernel interface architecture-abstraction header
 * (include/arch/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_SPARC_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_SPARC_ARCH_H_

#include <irq.h>
#include <arch/sparc/exp.h>
#include <arch/sparc/thread.h>
#include <arch/sparc/asm_inline.h>
#include <devicetree.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* stack areas for each thread (could be 4, but include several mergins) */
#define STACK_ALIGN (0x100)

/* CPU specific requirement for %sp alignment */
#define STACK_ALIGN_SIZE 8

#define PSR_ICC (0xf << 20)
#define PSR_EC  BIT(13)
#define PSR_EF  BIT(12)
#define PSR_PIL (0xf << 8)
#define PSR_S   BIT(7)
#define PSR_PS  BIT(6)
#define PSR_ET  BIT(5)
#define PSR_CWP (0x1f)

/* We start with 7th window register and use RETT to make CWP 0 to
 * start a thread.  This allows one and only one trap.  In other
 * words, we don't support nested interrupt yet.
 */
#define INIT_WIM     (BIT(1))
#define INIT_PSR_CWP (0x7 << 0)
#define INIT_PSR     (PSR_PIL | PSR_S | PSR_PS | PSR_ET | INIT_PSR_CWP)

#ifndef _ASMLANGUAGE

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key;
	unsigned int tmp;

	__asm__ volatile(
		"rd	%%psr, %0\n\t"       /* store psr in return val */
		"or	%0, %2, %1\n\t"      /* set 1s to all PIL fields */
		"wr	%1, %%g0, %%psr\n\t" /* write back to psr */
		"nop; nop; nop;"             /* psr is delayed write */
		: "=&r" (key), "=r" (tmp)
		: "i" (PSR_PIL)
		: "memory");

	return key;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	unsigned int tmp;

	__asm__ volatile(
		"and	%1, %2, %1\n\t"    /* take PIL filed from the key */
		"rd	%%psr, %0\n\t"     /* get the current psr */
		"andn	%0, %2, %0\n\t"    /* get rid of PIL field */
		"wr	%0, %1, %%psr\n\t" /* write back with xor %0 and %1 */
		"nop; nop; nop;"           /* psr is delayed write */
		: "=&r" (tmp)
		: "r" (key), "i" (PSR_PIL)
		: "memory");
}

/**
 * Returns true if interrupts were unlocked prior to the
 * arch_irq_lock() call that produced the key argument.
 */
static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return (key & PSR_PIL) == 0;
}

static inline u32_t arch_k_cycle_get_32(void)
{
	return 0;
}

/**
 * @brief Explicitly nop operation.
 */
static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_SPARC_ARCH_H_ */
