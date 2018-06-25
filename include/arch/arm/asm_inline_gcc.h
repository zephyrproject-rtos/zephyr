/* ARM Cortex-M GCC specific public inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Either public functions or macros or invoked by public functions */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_ASM_INLINE_GCC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <arch/arm/exc.h>
#include <irq.h>

/**
 *
 * @brief Disable all interrupts on the CPU
 *
 * This routine disables interrupts.  It can be called from either interrupt or
 * thread level.  This routine returns an architecture-dependent
 * lock-out key representing the "interrupt disable state" prior to the call;
 * this key can be passed to irq_unlock() to re-enable interrupts.
 *
 * The lock-out key should only be used as the argument to the irq_unlock()
 * API.  It should never be used to manually re-enable interrupts or to inspect
 * or manipulate the contents of the source register.
 *
 * This function can be called recursively: it will return a key to return the
 * state of interrupt locking to the previous level.
 *
 * WARNINGS
 * Invoking a kernel routine with interrupts locked may result in
 * interrupts being re-enabled for an unspecified period of time.  If the
 * called routine blocks, interrupts will be re-enabled while another
 * thread executes, or while the system is idle.
 *
 * The "interrupt disable state" is an attribute of a thread.  Thus, if a
 * thread disables interrupts and subsequently invokes a kernel
 * routine that causes the calling thread to block, the interrupt
 * disable state will be restored when the thread is later rescheduled
 * for execution.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 *
 * @internal
 *
 * On ARMv7-M and ARMv8-M Mainline CPUs, this function prevents regular
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


/**
 *
 * @brief Enable all interrupts on the CPU (inline)
 *
 * This routine re-enables interrupts on the CPU.  The @a key parameter is an
 * architecture-dependent lock-out key that is returned by a previous
 * invocation of irq_lock().
 *
 * This routine can be called from either interrupt or thread level.
 *
 * @param key architecture-dependent lock-out key
 *
 * @return N/A
 *
 * On Cortex-M0/M0+, this enables all interrupts if they were not
 * previously disabled.
 *
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

/**
 * Returns true if interrupts were unlocked prior to the
 * z_arch_irq_lock() call that produced the key argument.
 */
static ALWAYS_INLINE bool z_arch_irq_unlocked(unsigned int key)
{
	/* This convention works for both PRIMASK and BASEPRI */
	return key == 0;
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_ASM_INLINE_GCC_H_ */
