/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Runtime lock dependency validator
 *
 * When CONFIG_LOCKDEP is enabled the kernel records the order in which
 * mutexes and spinlocks are acquired and reports lock ordering patterns that
 * can deadlock. The validator is fully automatic and requires no changes to
 * code that uses the locks. This header only declares the hooks called from
 * the kernel locking primitives and the diagnostic interface used by tests.
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_LOCKDEP_H_
#define ZEPHYR_INCLUDE_DEBUG_LOCKDEP_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

struct k_thread;

/**
 * @cond INTERNAL_HIDDEN
 */

#ifdef CONFIG_LOCKDEP

#if defined(__GNUC__) || defined(__clang__)
#define Z_LOCKDEP_CALLER_ADDR ((uintptr_t)__builtin_return_address(0))
#else
#define Z_LOCKDEP_CALLER_ADDR 0U
#endif

enum z_lockdep_type {
	Z_LOCKDEP_MUTEX,
	Z_LOCKDEP_SPINLOCK,
};

struct z_lockdep_held_lock {
	const void *lock;
	uintptr_t acquire_ip;
	uint16_t recursion;
};

struct z_lockdep_thread_state {
	uint8_t depth;
	uint8_t overflow_depth;
	struct z_lockdep_held_lock held[CONFIG_LOCKDEP_MAX_HELD_LOCKS];
};

/* Hooks called from the kernel locking primitives. */
void z_lockdep_before_acquire(const void *lock, enum z_lockdep_type type, bool trylock,
			      uintptr_t acquire_ip);
void z_lockdep_after_acquire(const void *lock, enum z_lockdep_type type, bool trylock,
			     uintptr_t acquire_ip);
void z_lockdep_release(const void *lock, enum z_lockdep_type type);
void z_lockdep_release_unordered(const void *lock, enum z_lockdep_type type);
void z_lockdep_init_lock(const void *lock);
void z_lockdep_init_thread(struct k_thread *thread);

/* Diagnostic interface. Intended for kernel tests, not for application use. */

struct z_lockdep_stats {
	uint32_t lock_instances;
	uint32_t dependencies;
	uint32_t errors;
	uint32_t overflows;
};

/*
 * Test whether the current execution context holds a lock instance. Returns
 * true if held by the current thread (mutex) or CPU (spinlock). Safe to call
 * from ISR context.
 */
bool z_lockdep_is_held(const void *lock);

void z_lockdep_stats_get(struct z_lockdep_stats *stats);

/*
 * Reset all learned dependency state. Only safe when no context holds a
 * tracked lock or is using the validator.
 */
void z_lockdep_reset(void);

#endif /* CONFIG_LOCKDEP */

/**
 * INTERNAL_HIDDEN @endcond
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEBUG_LOCKDEP_H_ */
