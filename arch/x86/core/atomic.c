/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file Atomic ops for x86
 * DESCRIPTION This module provides the atomic operators for IA-32
 * architectures on platforms that support the LOCK prefix instruction.
 *
 * The atomic operations are guaranteed to be atomic with respect
 * to interrupt service routines, and to operations performed by peer
 * processors.
 */

#include <atomic.h>
#include <toolchain.h>

/**
 *
 * @brief Atomic compare-and-set primitive
 *
 * This routine provides the compare-and-set operator. If the original value at
 * <target> equals <oldValue>, then <newValue> is stored at <target> and the
 * function returns 1.
 *
 * If the original value at <target> does not equal <oldValue>, then the store
 * is not done and the function returns 0.
 *
 * The reading of the original value at <target>, the comparison,
 * and the write of the new value (if it occurs) all happen atomically with
 * respect to both interrupts and accesses of other processors to <target>.
 *
 * @param target address to be tested
 * @param oldValue value to compare against
 * @param newValue value to compare against
 * @return Returns 1 if <newValue> is written, 0 otherwise.
 */
FUNC_NO_FP int atomic_cas(atomic_t *target, atomic_val_t oldValue,
			  atomic_val_t newValue)
{
	int eax;

	__asm__ (
	    "movl %[oldValue], %%eax\n\t"
	    "lock cmpxchg %[newValue], (%[target])\n\t"
	    "sete %%al\n\t"
	    "movzbl %%al,%%eax"
	    : "=&a" (eax)
	    : [newValue] "r" (newValue), [oldValue] "m" (oldValue),
	      [target] "r" (target));

	return eax;
}

/**
 *
 * @brief Atomic addition primitive
 *
 * This routine provides the atomic addition operator. The <value> is
 * atomically added to the value at <target>, placing the result at <target>,
 * and the old value from <target> is returned.
 *
 * @param target memory location to add to
 * @param value the value to add
 *
 * @return The previous value from <target>
 */
FUNC_NO_FP atomic_val_t atomic_add(atomic_t *target, atomic_val_t value)
{
	__asm__ ("lock xadd %[value], (%[target])"
		 : [value] "+r" (value)
		 : [target] "r" (target)
		 : );
	return value;
}

/**
 *
 * @brief Atomic subtraction primitive
 *
 * This routine provides the atomic subtraction operator. The <value> is
 * atomically subtracted from the value at <target>, placing the result at
 * <target>, and the old value from <target> is returned.
 *
 * @param target the memory location to subtract from
 * @param value the value to subtract
 *
 * @return The previous value from <target>
 */
FUNC_NO_FP atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value)
{
	__asm__ ("neg %[value]\n\t"
		 "lock xadd %[value], (%[target])"
		  : [value] "+r" (value)
		  : [target] "r" (target)
		  : );
	return value;
}

/**
 *
 * @brief Atomic increment primitive
 *
 * @param target memory location to increment
 *
 * This routine provides the atomic increment operator. The value at <target>
 * is atomically incremented by 1, and the old value from <target> is returned.
 *
 * @return The value from <target> before the increment
 */
FUNC_NO_FP atomic_val_t atomic_inc(atomic_t *target)
{
	atomic_t value = 1;

	__asm__ ("lock xadd %[value], (%[target])"
		 : [value] "+r" (value)
		 : [target] "r" (target));
	return value;
}

/**
 *
 * @brief Atomic decrement primitive
 *
 * @param target memory location to decrement
 *
 * This routine provides the atomic decrement operator. The value at <target>
 * is atomically decremented by 1, and the old value from <target> is returned.
 *
 * @return The value from <target> prior to the decrement
 */
FUNC_NO_FP atomic_val_t atomic_dec(atomic_t *target)
{
	atomic_t value = -1;

	__asm__ ("lock xadd %[value], (%[target])"
		 : [value] "+r" (value)
		 : [target] "r" (target));
	return value;
}

/**
 *
 * @brief Atomic get primitive
 *
 * @param target memory location to read from
 *
 * This routine provides the atomic get primitive to atomically read
 * a value from <target>. It simply does an ordinary load.  Note that <target>
 * is expected to be aligned to a 4-byte boundary.
 *
 * @return The value read from <target>
 */
FUNC_NO_FP atomic_val_t atomic_get(const atomic_t *target)
{
	return *target;
}

/**
 *
 * @brief Atomic get-and-set primitive
 *
 * This routine provides the atomic set operator. The <value> is atomically
 * written at <target> and the previous value at <target> is returned.
 *
 * @param target the memory location to write to
 * @param value the value to write
 *
 * @return The previous value from <target>
 */
FUNC_NO_FP atomic_val_t atomic_set(atomic_t *target, atomic_val_t value)
{
	/*
	 * The 'lock' prefix is not required with the 'xchg' instruction.
	 * According to the IA-32 instruction reference manual:
	 *
	 *   "If a memory operand is referenced, the processor's locking
	 *    protocol is automatically implemented for the duration of
	 *    the exchange operation, regardless of the presence
	 *    or absence of the LOCK prefix or of the value of the IOPL."
	 */
	__asm__ ("xchg %[value], (%[target])"
		 : [value] "+r" (value)
		 : [target] "r" (target));

	return value;
}

/**
 *
 * @brief Atomic clear primitive
 *
 * This routine provides the atomic clear operator. The value of 0 is atomically
 * written at <target> and the previous value at <target> is returned. (Hence,
 * atomic_clear(pAtomicVar) is equivalent to atomic_set(pAtomicVar, 0).)
 *
 * @param target the memory location to write
 *
 * @return The previous value from <target>
 */
FUNC_NO_FP atomic_val_t atomic_clear(atomic_t *target)
{
	atomic_t value = 0;
	/* See note in atomic_set about non-use of 'lock' here */
	__asm__ ("xchg %[value], (%[target])"
		 : [value] "+r" (value)
		 : [target] "r" (target));

	return value;
}

/**
 *
 * @brief Atomic bitwise inclusive OR primitive
 *
 * This routine provides the atomic bitwise inclusive OR operator. The <value>
 * is atomically bitwise OR'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * @param target the memory location to be modified
 * @param value the value to OR
 *
 * @return The previous value from <target>
 */
FUNC_NO_FP atomic_val_t atomic_or(atomic_t *target, atomic_val_t value)
{
	atomic_val_t eax;

	__asm__ volatile (
	    "mov %[target], %%edx\n\t"
	    /*
	     * Dereference target pointer and store in EAX, we will
	     * use this later to ensure the value hasn't changed
	     */
	    "mov (%%edx), %%eax\n\t"
	    "1:\n\t"
	    /*
	     * Set ECX to be (value <op> *eax), use ECX so we don't lose
	     * the original value in case we need to do this again
	     */
	    "mov %[value], %%ecx\n\t"
	    "or %%eax, %%ecx\n\t"
	    /*
	     * Check if *EDX (which is *target) == EAX
	     * If they differ, *target was changed, EAX gets updated with
	     * the new value of *target, and we try again
	     * If they are the same, EAX now has ECX's value which is
	     * what we want to return to the caller.
	     */
	    "lock cmpxchg %%ecx, (%%edx)\n\t"
	    "jnz 1b"
	    : "=a" (eax)
	    : [target] "m" (target), [value] "m" (value)
	    : "ecx", "edx");

	return eax;
}

/**
 *
 * @brief Atomic bitwise exclusive OR (XOR) primitive
 *
 * This routine provides the atomic bitwise exclusive OR operator. The <value>
 * is atomically bitwise XOR'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * @param target the memory location to be modified
 * @param value the value to XOR
 *
 * @return The previous value from <target>
 */
FUNC_NO_FP atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value)
{
	/*
	 * See comments in atomic_or() for explanation on how
	 * this works
	 */
	atomic_val_t eax;

	__asm__ volatile (
	    "mov %[target], %%edx\n\t"
	    "mov (%%edx), %%eax\n\t"
	    "1:\n\t"
	    "mov %[value], %%ecx\n\t"
	    "xor %%eax, %%ecx\n\t"
	    "lock cmpxchg %%ecx, (%%edx)\n\t"
	    "jnz 1b"
	    : "=a" (eax)
	    : [target] "m" (target), [value] "m" (value)
	    : "ecx", "edx");

	return eax;
}

/**
 *
 * @brief Atomic bitwise AND primitive
 *
 * This routine provides the atomic bitwise AND operator. The <value> is
 * atomically bitwise AND'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * @param target the memory location to be modified
 * @param value the value to AND
 *
 * @return The previous value from <target>
 */
FUNC_NO_FP atomic_val_t atomic_and(atomic_t *target, atomic_val_t value)
{
	/*
	 * See comments in atomic_or() for explanation on how
	 * this works
	 */
	atomic_val_t eax;

	__asm__ volatile (
	    "mov %[target], %%edx\n\t"
	    "mov (%%edx), %%eax\n\t"
	    "1:\n\t"
	    "mov %[value], %%ecx\n\t"
	    "and %%eax, %%ecx\n\t"
	    "lock cmpxchg %%ecx, (%%edx)\n\t"
	    "jnz 1b"
	    : "=a" (eax)
	    : [target] "m" (target), [value] "m" (value)
	    : "ecx", "edx");

	return eax;
}

/**
 *
 * @brief Atomic bitwise NAND primitive
 *
 * This routine provides the atomic bitwise NAND operator. The <value> is
 * atomically bitwise NAND'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * @param target the memory location to be modified
 * @param value the value to NAND
 *
 * @return The previous value from <target>
 */
FUNC_NO_FP atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value)
{
	/*
	 * See comments in atomic_or() for explanation on how
	 * this works
	 */
	atomic_val_t eax;

	__asm__ volatile (
	    "mov %[target], %%edx\n\t"
	    "mov (%%edx), %%eax\n\t"
	    "1:\n\t"
	    "mov %[value], %%ecx\n\t"
	    "and %%eax, %%ecx\n\t"
	    "not %%ecx\n\t"
	    "lock cmpxchg %%ecx, (%%edx)\n\t"
	    "jnz 1b"
	    : "=a" (eax)
	    : [target] "m" (target), [value] "m" (value)
	    : "ecx", "edx");

	return eax;
}
