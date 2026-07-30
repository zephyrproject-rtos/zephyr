/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_M68K_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_M68K_ARCH_H_

#define ARCH_STACK_PTR_ALIGN 2

#include <zephyr/arch/common/ffs.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/m68k/exception.h>
#include <zephyr/arch/m68k/thread.h>

#ifndef _ASMLANGUAGE

#include <zephyr/sw_isr_table.h>

extern uint32_t sys_clock_cycle_get_32(void);
extern uint64_t sys_clock_cycle_get_64(void);

static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

static inline uint64_t arch_k_cycle_get_64(void)
{
	return sys_clock_cycle_get_64();
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

#define M68K_SR_IPL_MASK 0x0700U

static ALWAYS_INLINE uint16_t z_m68k_read_sr(void)
{
	uint16_t sr;

	__asm__ volatile(
		"move.w %%sr,%0"
		: "=d"(sr)
		:
		: "memory");

	return sr;
}

/* CPU vectors have no per-line priority or flags. */
#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)\
	{								\
		Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p);		\
	}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key;

	__asm__ volatile(
		"move.w %%sr,%0\n\t"
		"ori.w #0x0700,%%sr\n\t"
		"andi.l #0x00000700,%0"
		: "=d"(key)
		:
		: "memory", "cc");

	return key;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	uint16_t scratch;

	__asm__ volatile(
		"move.w %%sr,%0\n\t"
		"andi.w #0xf8ff,%0\n\t"
		"or.w %1,%0\n\t"
		"move.w %0,%%sr"
		: "=&d"(scratch)
		: "d"(key)
		: "memory", "cc");
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return !key;
}

static ALWAYS_INLINE bool arch_cpu_irqs_are_enabled(void)
{
	return !(z_m68k_read_sr() & M68K_SR_IPL_MASK);
}

#define ARCH_EXCEPT(reason_p)						\
	do {								\
		register unsigned int reason __asm__("d0") = (reason_p);	\
									\
		__asm__ volatile(						\
			"trap #14"					\
			: "+d"(reason)					\
			:						\
			: "memory");					\
	} while (false)

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_M68K_ARCH_H_ */
