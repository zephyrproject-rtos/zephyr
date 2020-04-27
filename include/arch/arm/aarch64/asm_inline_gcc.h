/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Either public functions or macros or invoked by public functions */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_ASM_INLINE_GCC_H_

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#ifndef _ASMLANGUAGE

#include <arch/arm/aarch64/cpu.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE void __DSB(void)
{
	__asm__ volatile ("dsb sy" : : : "memory");
}

static ALWAYS_INLINE void __DMB(void)
{
	__asm__ volatile ("dmb sy" : : : "memory");
}

static ALWAYS_INLINE void __ISB(void)
{
	__asm__ volatile ("isb" : : : "memory");
}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key;

	/*
	 * Return the whole DAIF register as key but use DAIFSET to disable
	 * IRQs.
	 */
	__asm__ volatile("mrs %0, daif;"
			 "msr daifset, %1;"
			 "isb"
			 : "=r" (key)
			 : "i" (DAIFSET_IRQ)
			 : "memory", "cc");

	return key;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	__asm__ volatile("msr daif, %0;"
			 "isb"
			 :
			 : "r" (key)
			 : "memory", "cc");
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	/* We only check the (I)RQ bit on the DAIF register */
	return (key & DAIF_IRQ) == 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_ASM_INLINE_GCC_H_ */
