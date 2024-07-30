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

#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/kernel_structs.h>

/* Single global spinlock for atomic operations.  This is fallback
 * code, not performance sensitive.  At least by not using irq_lock()
 * in SMP contexts we won't content with legitimate users of the
 * global lock.
 */
static struct k_spinlock lock;

/* For those rare CPUs which support user mode, but not native atomic
 * operations, the best we can do for them is implement the atomic
 * functions as system calls, since in user mode locking a spinlock is
 * forbidden.
 */
#ifdef CONFIG_USERSPACE
#include <zephyr/internal/syscall_handler.h>

#define ATOMIC_SYSCALL_HANDLER_TARGET(name) \
	static inline atomic_val_t z_vrfy_##name(atomic_t *target) \
	{								\
		K_OOPS(K_SYSCALL_MEMORY_WRITE(target, sizeof(atomic_t))); \
		return z_impl_##name((atomic_t *)target); \
	}

#define ATOMIC_SYSCALL_HANDLER_TARGET_VALUE(name) \
	static inline atomic_val_t z_vrfy_##name(atomic_t *target, \
						 atomic_val_t value) \
	{								\
		K_OOPS(K_SYSCALL_MEMORY_WRITE(target, sizeof(atomic_t))); \
		return z_impl_##name((atomic_t *)target, value); \
	}
#else
#define ATOMIC_SYSCALL_HANDLER_TARGET(name)
#define ATOMIC_SYSCALL_HANDLER_TARGET_VALUE(name)
#endif /* CONFIG_USERSPACE */

/**
 *
 * @brief Atomic compare-and-set primitive
 *
 * This routine provides the compare-and-set operator. If the original value at
 * <target> equals <oldValue>, then <newValue> is stored at <target> and the
 * function returns true.
 *
 * If the original value at <target> does not equal <oldValue>, then the store
 * is not done and the function returns false.
 *
 * The reading of the original value at <target>, the comparison,
 * and the write of the new value (if it occurs) all happen atomically with
 * respect to both interrupts and accesses of other processors to <target>.
 *
 * @param target address to be tested
 * @param old_value value to compare against
 * @param new_value value to compare against
 * @return Returns true if <new_value> is written, false otherwise.
 */
bool z_impl_atomic_cas(atomic_t *target, atomic_val_t old_value,
		       atomic_val_t new_value)
{
	k_spinlock_key_t key;
	int ret = false;

	/*
	 * On SMP the k_spin_lock() definition calls atomic_cas().
	 * Using k_spin_lock() here would create an infinite loop and
	 * massive stack overflow. Consider CONFIG_ATOMIC_OPERATIONS_ARCH
	 * or CONFIG_ATOMIC_OPERATIONS_BUILTIN instead.
	 */
	BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP));

	key = k_spin_lock(&lock);

	if (*target == old_value) {
		*target = new_value;
		ret = true;
	}

	k_spin_unlock(&lock, key);

	return ret;
}

#ifdef CONFIG_USERSPACE
bool z_vrfy_atomic_cas(atomic_t *target, atomic_val_t old_value,
		       atomic_val_t new_value)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(target, sizeof(atomic_t)));

	return z_impl_atomic_cas((atomic_t *)target, old_value, new_value);
}
#include <zephyr/syscalls/atomic_cas_mrsh.c>
#endif /* CONFIG_USERSPACE */

bool z_impl_atomic_ptr_cas(atomic_ptr_t *target, atomic_ptr_val_t old_value,
			   atomic_ptr_val_t new_value)
{
	k_spinlock_key_t key;
	int ret = false;

	key = k_spin_lock(&lock);

	if (*target == old_value) {
		*target = new_value;
		ret = true;
	}

	k_spin_unlock(&lock, key);

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline bool z_vrfy_atomic_ptr_cas(atomic_ptr_t *target,
					 atomic_ptr_val_t old_value,
					 atomic_ptr_val_t new_value)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(target, sizeof(atomic_ptr_t)));

	return z_impl_atomic_ptr_cas(target, old_value, new_value);
}
#include <zephyr/syscalls/atomic_ptr_cas_mrsh.c>
#endif /* CONFIG_USERSPACE */

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
atomic_val_t z_impl_atomic_add(atomic_t *target, atomic_val_t value)
{
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target += value;

	k_spin_unlock(&lock, key);

	return ret;
}

ATOMIC_SYSCALL_HANDLER_TARGET_VALUE(atomic_add);

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
atomic_val_t z_impl_atomic_sub(atomic_t *target, atomic_val_t value)
{
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target -= value;

	k_spin_unlock(&lock, key);

	return ret;
}

ATOMIC_SYSCALL_HANDLER_TARGET_VALUE(atomic_sub);

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

atomic_ptr_val_t atomic_ptr_get(const atomic_ptr_t *target)
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
atomic_val_t z_impl_atomic_set(atomic_t *target, atomic_val_t value)
{
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target = value;

	k_spin_unlock(&lock, key);

	return ret;
}

ATOMIC_SYSCALL_HANDLER_TARGET_VALUE(atomic_set);

atomic_ptr_val_t z_impl_atomic_ptr_set(atomic_ptr_t *target,
				       atomic_ptr_val_t value)
{
	k_spinlock_key_t key;
	atomic_ptr_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target = value;

	k_spin_unlock(&lock, key);

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline atomic_ptr_val_t z_vrfy_atomic_ptr_set(atomic_ptr_t *target,
						     atomic_ptr_val_t value)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(target, sizeof(atomic_ptr_t)));

	return z_impl_atomic_ptr_set(target, value);
}
#include <zephyr/syscalls/atomic_ptr_set_mrsh.c>
#endif /* CONFIG_USERSPACE */

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
atomic_val_t z_impl_atomic_or(atomic_t *target, atomic_val_t value)
{
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target |= value;

	k_spin_unlock(&lock, key);

	return ret;
}

ATOMIC_SYSCALL_HANDLER_TARGET_VALUE(atomic_or);

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
atomic_val_t z_impl_atomic_xor(atomic_t *target, atomic_val_t value)
{
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target ^= value;

	k_spin_unlock(&lock, key);

	return ret;
}

ATOMIC_SYSCALL_HANDLER_TARGET_VALUE(atomic_xor);

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
atomic_val_t z_impl_atomic_and(atomic_t *target, atomic_val_t value)
{
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target &= value;

	k_spin_unlock(&lock, key);

	return ret;
}

ATOMIC_SYSCALL_HANDLER_TARGET_VALUE(atomic_and);

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
atomic_val_t z_impl_atomic_nand(atomic_t *target, atomic_val_t value)
{
	k_spinlock_key_t key;
	atomic_val_t ret;

	key = k_spin_lock(&lock);

	ret = *target;
	*target = ~(*target & value);

	k_spin_unlock(&lock, key);

	return ret;
}

ATOMIC_SYSCALL_HANDLER_TARGET_VALUE(atomic_nand);

#ifdef CONFIG_USERSPACE
#include <zephyr/syscalls/atomic_add_mrsh.c>
#include <zephyr/syscalls/atomic_sub_mrsh.c>
#include <zephyr/syscalls/atomic_set_mrsh.c>
#include <zephyr/syscalls/atomic_or_mrsh.c>
#include <zephyr/syscalls/atomic_xor_mrsh.c>
#include <zephyr/syscalls/atomic_and_mrsh.c>
#include <zephyr/syscalls/atomic_nand_mrsh.c>
#endif /* CONFIG_USERSPACE */
