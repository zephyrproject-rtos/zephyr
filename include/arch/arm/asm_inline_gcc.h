/* ARM Cortex-M GCC specific public inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Either public functions or macros or invoked by public functions */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_ASM_INLINE_GCC_H_

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <arch/arm/exc.h>
#include <irq.h>

#ifdef __cplusplus
extern "C" {
#endif

/* On ARMv7-M and ARMv8-M Mainline CPUs, this function prevents regular
 * exceptions (i.e. with interrupt priority lower than or equal to
 * _EXC_IRQ_DEFAULT_PRIO) from interrupting the CPU. NMI, Faults, SVC,
 * and Zero Latency IRQs (if supported) may still interrupt the CPU.
 *
 * On ARMv6-M and ARMv8-M Baseline CPUs, this function reads the value of
 * PRIMASK which shows if interrupts are enabled, then disables all interrupts
 * except NMI.
 */

static ALWAYS_INLINE unsigned int z_arch_irq_lock(void)
{
	unsigned int key;

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	__asm__ volatile("mrs %0, PRIMASK;"
		"cpsid i"
		: "=r" (key)
		:
		: "memory");
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	unsigned int tmp;

	__asm__ volatile(
		"mov %1, %2;"
		"mrs %0, BASEPRI;"
		"msr BASEPRI, %1;"
		"isb;"
		: "=r"(key), "=r"(tmp)
		: "i"(_EXC_IRQ_DEFAULT_PRIO)
		: "memory");
#elif defined(CONFIG_ARMV7_R)
	__asm__ volatile("mrs %0, cpsr;"
		"cpsid i"
		: "=r" (key)
		:
		: "memory", "cc");
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

	return key;
}


/* On Cortex-M0/M0+, this enables all interrupts if they were not
 * previously disabled.
 */

static ALWAYS_INLINE void z_arch_irq_unlock(unsigned int key)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	if (key) {
		return;
	}
	__asm__ volatile(
		"cpsie i;"
		"isb"
		: : : "memory");
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__asm__ volatile(
		"msr BASEPRI, %0;"
		"isb;"
		:  : "r"(key) : "memory");
#elif defined(CONFIG_ARMV7_R)
	__asm__ volatile("msr cpsr_c, %0"
			:
			: "r" (key)
			: "memory", "cc");
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
}

static ALWAYS_INLINE bool z_arch_irq_unlocked(unsigned int key)
{
	/* This convention works for both PRIMASK and BASEPRI */
	return key == 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_ASM_INLINE_GCC_H_ */
