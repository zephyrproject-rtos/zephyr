/*
 * Copyright (c) 2018 SiFive, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <atomic.h>

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
atomic_val_t atomic_add(atomic_t *target, atomic_val_t value)
{
	atomic_val_t old_value;

	__asm__ volatile("amoadd.w %[old], %[incr], (%[atomic])"
			: [old] "=r" (old_value)
			: [atomic] "r" (target), [incr] "r" (value)
			: "memory");

	return old_value;
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
atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value)
{
	return atomic_add(target, -value);
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
atomic_val_t atomic_inc(atomic_t *target)
{
	return atomic_add(target, 1);
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
atomic_val_t atomic_dec(atomic_t *target)
{
	return atomic_add(target, -1);
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
atomic_val_t atomic_get(const atomic_t *target)
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
atomic_val_t atomic_set(atomic_t *target, atomic_val_t value)
{
	atomic_val_t old_value;

	__asm__ volatile("amoswap.w %[old], %[new], (%[mem])"
			: [old] "=r" (old_value)
			: [new] "r" (value), [mem] "r" (target)
			: "memory");

	return old_value;
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
atomic_val_t atomic_clear(atomic_t *target)
{
	return atomic_set(target, 0);
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
atomic_val_t atomic_or(atomic_t *target, atomic_val_t value)
{
	atomic_val_t old_value;

	__asm__ volatile("amoor.w %[old], %[new], (%[mem])"
			: [old] "=r" (old_value)
			: [new] "r" (value), [mem] "r" (target)
			: "memory");

	return old_value;
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
atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value)
{
	atomic_val_t old_value;

	__asm__ volatile("amoxor.w %[old], %[new], (%[mem])"
			: [old] "=r" (old_value)
			: [new] "r" (value), [mem] "r" (target)
			: "memory");

	return old_value;
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
atomic_val_t atomic_and(atomic_t *target, atomic_val_t value)
{
	atomic_val_t old_value;

	__asm__ volatile("amoand.w %[old], %[new], (%[mem])"
			: [old] "=r" (old_value)
			: [new] "r" (value), [mem] "r" (target)
			: "memory");

	return old_value;
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
atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value)
{
	/* TODO there's no atomic NAND instruction in RISC-V. What other
	 * solutions are appropriate? */
	atomic_val_t old_value = *target;

	*target = ~(old_value & value);

	return old_value;
}
