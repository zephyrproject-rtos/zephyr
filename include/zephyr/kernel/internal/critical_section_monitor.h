/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal IRQ and spinlock timing hooks for the critical section monitor.
 * @ingroup internal_api
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_INTERNAL_CRITICAL_SECTION_MONITOR_H_
#define ZEPHYR_INCLUDE_KERNEL_INTERNAL_CRITICAL_SECTION_MONITOR_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/toolchain.h>

struct k_spinlock;

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#ifdef CONFIG_CRITICAL_SECTION_MONITOR

extern bool z_critical_section_monitor_ready;

static ALWAYS_INLINE bool z_critical_section_monitor_is_ready(void)
{
	return z_critical_section_monitor_ready;
}

#if defined(__GNUC__) || defined(__clang__)
#define Z_CRITICAL_SECTION_MONITOR_CALLER()                                                        \
	((uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)))
#else
#define Z_CRITICAL_SECTION_MONITOR_CALLER() 0U
#endif

/* Measure IRQ intervals with local interrupts locked, including scheduler handoffs. */
__noinline void z_irq_timing_begin(unsigned int key, uintptr_t caller);
__noinline void z_irq_timing_end(unsigned int key);
__noinline void z_irq_timing_idle_enter(void);

#ifdef CONFIG_SMP
__noinline void z_spinlock_timing_abort(const struct k_spinlock *lock, unsigned int key);
#endif /* CONFIG_SMP */
/* Consume the hold timestamp shared with the spinlock time-limit check. */
__noinline void z_critical_section_monitor_spin_acquired(const struct k_spinlock *lock,
							 unsigned int key, uint32_t now);
/* Publish after releasing the lock, but before restoring local interrupts. */
__noinline void z_spinlock_timing_unlocked(const struct k_spinlock *lock, unsigned int key,
					 uint32_t now);
/* k_spin_release() ends only the hold interval, leaving IRQ timing active. */
__noinline void z_spinlock_timing_released(const struct k_spinlock *lock, uint32_t now);

#ifndef CONFIG_SMP
__noinline unsigned int z_critical_section_monitor_irq_lock(void);
__noinline void z_critical_section_monitor_irq_unlock(unsigned int key);
#endif /* !CONFIG_SMP */

#else /* CONFIG_CRITICAL_SECTION_MONITOR */

#define Z_CRITICAL_SECTION_MONITOR_CALLER() 0U
#define z_critical_section_monitor_is_ready() false

/* Empty inline functions can still retain argument storage at -O0. */
#define z_irq_timing_begin(key, caller) do { } while (false)
#define z_irq_timing_end(key) do { } while (false)
#define z_irq_timing_idle_enter() do { } while (false)

#ifdef CONFIG_SMP
#define z_spinlock_timing_abort(lock, key) do { } while (false)
#endif /* CONFIG_SMP */

#define z_critical_section_monitor_spin_acquired(lock, key, now) do { } while (false)
#define z_spinlock_timing_unlocked(lock, key, now) do { } while (false)
#define z_spinlock_timing_released(lock, now) do { } while (false)

#endif /* CONFIG_CRITICAL_SECTION_MONITOR */

#if defined(CONFIG_CRITICAL_SECTION_MONITOR) && defined(CONFIG_SMP)
__noinline void z_spinlock_timing_attempt(const struct k_spinlock *lock, unsigned int key);
#else
#define z_spinlock_timing_attempt(lock, key) do { } while (false)
#endif

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KERNEL_INTERNAL_CRITICAL_SECTION_MONITOR_H_ */
