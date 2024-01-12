/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ATOMIC_C_H_
#define ZEPHYR_INCLUDE_SYS_ATOMIC_C_H_

/* Included from <atomic.h> */

#ifdef __cplusplus
extern "C" {
#endif

/* Simple and correct (but very slow) implementation of atomic
 * primitives that require nothing more than kernel interrupt locking.
 */

__syscall bool atomic_cas(atomic_t *target, atomic_val_t old_value,
			 atomic_val_t new_value);

__syscall bool atomic_ptr_cas(atomic_ptr_t *target, atomic_ptr_val_t old_value,
			      atomic_ptr_val_t new_value);

__syscall atomic_val_t atomic_add(atomic_t *target, atomic_val_t value);

__syscall atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value);

static inline atomic_val_t atomic_inc(atomic_t *target)
{
	return atomic_add(target, 1);

}

static inline atomic_val_t atomic_dec(atomic_t *target)
{
	return atomic_sub(target, 1);

}

atomic_val_t atomic_get(const atomic_t *target);

atomic_ptr_val_t atomic_ptr_get(const atomic_ptr_t *target);

__syscall atomic_val_t atomic_set(atomic_t *target, atomic_val_t value);

__syscall atomic_ptr_val_t atomic_ptr_set(atomic_ptr_t *target, atomic_ptr_val_t value);

static inline atomic_val_t atomic_clear(atomic_t *target)
{
	return atomic_set(target, 0);

}

static inline atomic_ptr_val_t atomic_ptr_clear(atomic_ptr_t *target)
{
	return atomic_ptr_set(target, NULL);

}

__syscall atomic_val_t atomic_or(atomic_t *target, atomic_val_t value);

__syscall atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value);

__syscall atomic_val_t atomic_and(atomic_t *target, atomic_val_t value);

__syscall atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value);

#ifdef __cplusplus
}
#endif

#ifdef CONFIG_ATOMIC_OPERATIONS_C

#ifndef DISABLE_SYSCALL_TRACING
/* Skip defining macros of atomic_*() for syscall tracing.
 * Compiler does not like "({ ... tracing code ... })" and complains
 *
 *   error: expected identifier or '(' before '{' token
 *
 * ... even though there is a '(' before '{'.
 */
#define DISABLE_SYSCALL_TRACING
#define _REMOVE_DISABLE_SYSCALL_TRACING
#endif

#include <syscalls/atomic_c.h>

#ifdef _REMOVE_DISABLE_SYSCALL_TRACING
#undef DISABLE_SYSCALL_TRACING
#undef _REMOVE_DISABLE_SYSCALL_TRACING
#endif

#endif

#endif /* ZEPHYR_INCLUDE_SYS_ATOMIC_C_H_ */
