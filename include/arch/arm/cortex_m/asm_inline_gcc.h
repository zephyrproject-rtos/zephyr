/* ARM Cortex-M GCC specific public inline assembler functions and macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Either public functions or macros or invoked by public functions */

#ifndef _ASM_INLINE_GCC_PUBLIC_GCC_H
#define _ASM_INLINE_GCC_PUBLIC_GCC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#ifdef _ASMLANGUAGE

#define _SCS_BASE_ADDR _PPB_INT_SCS
#define _SCS_ICSR (_SCS_BASE_ADDR + 0xd04)
#define _SCS_ICSR_PENDSV (1 << 28)
#define _SCS_ICSR_UNPENDSV (1 << 27)
#define _SCS_ICSR_RETTOBASE (1 << 11)

#else /* !_ASMLANGUAGE */
#include <stdint.h>
#include <arch/arm/cortex_m/exc.h>
#include <irq.h>

/**
 *
 * @brief find most significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the most significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return most significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_msb_set(uint32_t op)
{
	if (!op) {
		return 0;
	}

	return 32 - __builtin_clz(op);
}


/**
 *
 * @brief find least significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the least significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return least significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_lsb_set(uint32_t op)
{
	return __builtin_ffs(op);
}


/**
 *
 * @brief Disable all interrupts on the CPU
 *
 * This routine disables interrupts.  It can be called from either interrupt,
 * task or fiber level.  This routine returns an architecture-dependent
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
 * fiber or task disables interrupts and subsequently invokes a kernel
 * routine that causes the calling thread to block, the interrupt
 * disable state will be restored when the thread is later rescheduled
 * for execution.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 *
 * @internal
 *
 * On Cortex-M3/M4, this function prevents exceptions of priority lower than
 * the two highest priorities from interrupting the CPU.
 *
 * On Cortex-M0/M0+, this function reads the value of PRIMASK which shows
 * if interrupts are enabled, then disables all interrupts except NMI.
 *
 */

static ALWAYS_INLINE unsigned int _arch_irq_lock(void)
{
	unsigned int key;

#if defined(CONFIG_ARMV6_M)
	__asm__ volatile("mrs %0, PRIMASK;"
		"cpsid i"
		: "=r" (key)
		:
		: "memory");
#elif defined(CONFIG_ARMV7_M)
	unsigned int tmp;

	__asm__ volatile(
		"mov %1, %2;"
		"mrs %0, BASEPRI;"
		"msr BASEPRI, %1"
		: "=r"(key), "=r"(tmp)
		: "i"(_EXC_IRQ_DEFAULT_PRIO)
		: "memory");
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

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
 * This routine can be called from either interrupt, task or fiber level.
 *
 * @param key architecture-dependent lock-out key
 *
 * @return N/A
 *
 * On Cortex-M0/M0+, this enables all interrupts if they were not
 * previously disabled.
 *
 */

static ALWAYS_INLINE void _arch_irq_unlock(unsigned int key)
{
#if defined(CONFIG_ARMV6_M)
	if (key) {
		return;
	}
	__asm__ volatile("cpsie i" : : : "memory");
#elif defined(CONFIG_ARMV7_M)
	__asm__ volatile("msr BASEPRI, %0" :  : "r"(key) : "memory");
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
}


#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ASM_INLINE_GCC_PUBLIC_GCC_H */
