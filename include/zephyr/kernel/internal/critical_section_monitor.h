/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal critical section monitor hooks.
 * @ingroup internal_api
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_INTERNAL_CRITICAL_SECTION_MONITOR_H_
#define ZEPHYR_INCLUDE_KERNEL_INTERNAL_CRITICAL_SECTION_MONITOR_H_

#include <stdint.h>

#include <zephyr/toolchain.h>

struct k_spinlock;

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#ifdef CONFIG_CRITICAL_SECTION_MONITOR

#if defined(__GNUC__) || defined(__clang__)
#define Z_CRITICAL_SECTION_MONITOR_CALLER()                                                        \
	((uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)))
#else
#define Z_CRITICAL_SECTION_MONITOR_CALLER() 0U
#endif

__noinline void z_critical_section_monitor_irq_start(unsigned int key,
						     const struct k_spinlock *spinlock,
						     uintptr_t caller);
__noinline void z_critical_section_monitor_irq_end(unsigned int key);
__noinline void z_critical_section_monitor_idle_enter(void);

__noinline void z_critical_section_monitor_spin_start(const struct k_spinlock *lock,
						      unsigned int key);
#ifdef CONFIG_SMP
__noinline void z_critical_section_monitor_spin_acquired(const struct k_spinlock *lock);
__noinline void z_critical_section_monitor_spin_abort(const struct k_spinlock *lock,
						      unsigned int key);
#endif /* CONFIG_SMP */
__noinline void z_critical_section_monitor_spin_unlock(const struct k_spinlock *lock,
						       unsigned int key);
__noinline void z_critical_section_monitor_spin_released(const struct k_spinlock *lock);

#ifndef CONFIG_SMP
__noinline unsigned int z_critical_section_monitor_irq_lock(void);
__noinline void z_critical_section_monitor_irq_unlock(unsigned int key);
#endif /* !CONFIG_SMP */

#endif /* CONFIG_CRITICAL_SECTION_MONITOR */

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KERNEL_INTERNAL_CRITICAL_SECTION_MONITOR_H_ */
