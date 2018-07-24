/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Atomic ops in pure C
 *
 * This module provides the atomic operators for processors
 * which do not support native atomic operations.
 *
 * The atomic operations are guaranteed to be atomic with respect
 * to interrupt service routines, and to operations performed by peer
 * processors.
 *
 * (originally from x86's atomic.c)
 */

#include <atomic.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <spinlock.h>

/* Single global spinlock for atomic operations.  This is fallback
 * code, not performance sensitive.  At least by not using irq_lock()
 * in SMP contexts we won't content with legitimate users of the
 * global lock.
 */
static struct k_spinlock lock;

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
 * @param old_value value to compare against
 * @param new_value value to compare against
 * @return Returns 1 if <new_value> is written, 0 otherwise.
 */
int atomic_cas(atomic_t *target, atomic_val_t old_value,
			  atomic_val_t new_value)
{
	k_spinlock_key_t key;
	int ret = 0;

	key = k_spin_lock(&lock);

	if (*target == old_value) {
		*target = new_value;
		ret = 1;
	}

	k_spin_unlock(&lock, key);

	return ret;
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
atomic_val_t atomic_add(atomic_t *target, atomic_val_t value)
{
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target += value;

	k_spin_unlock(&lock, key);

	return ret;
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
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target -= value;

	k_spin_unlock(&lock, key);

	return ret;
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
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	(*target)++;

	k_spin_unlock(&lock, key);

	return ret;
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
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	(*target)--;

	k_spin_unlock(&lock, key);

	return ret;
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
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target = value;

	k_spin_unlock(&lock, key);

	return ret;
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
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target = 0;

	k_spin_unlock(&lock, key);

	return ret;
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
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target |= value;

	k_spin_unlock(&lock, key);

	return ret;
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
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target ^= value;

	k_spin_unlock(&lock, key);

	return ret;
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
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target &= value;

	k_spin_unlock(&lock, key);

	return ret;
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
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target = ~(*target & value);

	k_spin_unlock(&lock, key);

	return ret;
}
