/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal IRQ and spinlock instrumentation hooks.
 * @ingroup internal_api
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_INTERNAL_CRITICAL_SECTION_HOOKS_H_
#define ZEPHYR_INCLUDE_KERNEL_INTERNAL_CRITICAL_SECTION_HOOKS_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/toolchain.h>

struct k_spinlock;

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#ifdef CONFIG_CRITICAL_SECTION_HOOKS

/*
 * The selected consumer implements these functions; the kernel calls them directly.
 * Calls can happen during boot, so ignore events until the consumer is ready.
 *
 * Hooks run with local IRQs disabled. Do not block, allocate memory, or call
 * IRQ/spinlock APIs that call these hooks again.
 */
#if defined(__GNUC__) || defined(__clang__)
#define Z_CRITICAL_SECTION_CALLER()                                                                \
	((uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)))
#else
#define Z_CRITICAL_SECTION_CALLER() 0U
#endif

/*
 * irq_lock() calls this hook even if local IRQs were already disabled.
 * key is the IRQ state before locking. caller identifies the call site.
 */
__noinline void z_crit_hook_irq_begin(unsigned int key, uintptr_t caller);
/*
 * irq_unlock() calls this before restoring IRQ state.
 * k_cpu_atomic_idle() calls it before entering idle.
 * The scheduler also calls it before switching threads. At that point,
 * _current may already point to the next thread.
 */
__noinline void z_crit_hook_irq_end(unsigned int key);
/* k_cpu_idle() calls this before entering idle. key is its original IRQ state. */
__noinline void z_crit_hook_irq_idle(unsigned int key);
/* Call z_crit_hook_irq_idle() with local IRQs disabled, then restore the IRQ state. */
__noinline void z_crit_irq_idle_enter(void);

/* Called after successful lock acquisition and validation. */
__noinline void z_crit_hook_spin_acquired(const struct k_spinlock *lock, unsigned int key);
/*
 * Called after releasing the lock, before restoring local IRQs.
 * Another CPU may have changed or freed the lock. Use its address only as
 * an ID; do not dereference it.
 */
__noinline void z_crit_hook_spin_unlocked(const struct k_spinlock *lock, unsigned int key);
/*
 * Called after k_spin_release() releases the lock; local IRQs stay disabled.
 * Use the lock address only as an ID; do not dereference it.
 */
__noinline void z_crit_hook_spin_released(const struct k_spinlock *lock);

#ifndef CONFIG_SMP
__noinline unsigned int z_crit_irq_lock(void);
__noinline void z_crit_irq_unlock(unsigned int key);
#endif /* !CONFIG_SMP */

#else /* CONFIG_CRITICAL_SECTION_HOOKS */

#define Z_CRITICAL_SECTION_CALLER() 0U

/* Use macros so disabled hooks generate no code, even without optimization. */
#define z_crit_hook_irq_begin(key, caller) do { } while (false)
#define z_crit_hook_irq_end(key) do { } while (false)
#define z_crit_irq_idle_enter() do { } while (false)

#define z_crit_hook_spin_acquired(lock, key) do { } while (false)
#define z_crit_hook_spin_unlocked(lock, key) do { } while (false)
#define z_crit_hook_spin_released(lock) do { } while (false)

#endif /* CONFIG_CRITICAL_SECTION_HOOKS */

#if defined(CONFIG_CRITICAL_SECTION_HOOKS) && defined(CONFIG_SMP)
/* Called before validation and the SMP lock attempt. */
__noinline void z_crit_hook_spin_attempt(const struct k_spinlock *lock, unsigned int key);
/* Called after k_spin_trylock() fails, before restoring local IRQs. */
__noinline void z_crit_hook_spin_abort(const struct k_spinlock *lock, unsigned int key);
#else
#define z_crit_hook_spin_attempt(lock, key) do { } while (false)
#define z_crit_hook_spin_abort(lock, key) do { } while (false)
#endif

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KERNEL_INTERNAL_CRITICAL_SECTION_HOOKS_H_ */
