/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KSPINLOCK_H_
#define ZEPHYR_KERNEL_INCLUDE_KSPINLOCK_H_

#include <zephyr/spinlock.h>
#include <stdbool.h>

extern struct k_spinlock _sched_spinlock;

/**
 * The intent of LOCK_SCHED_SPINLOCK is to lock the scheduler's spinlock for
 * the scope that follows (e.g. LOCK_SCHED_SPINLOCK { ... }). However, on
 * single core systems, the scheduler's spinlock is not needed when the locked
 * region would otherwise be encapsulated by another spinlock (as that spinlock
 * would degenerate into an IRQ lock). For that degenerate case, this macro
 * will expand to nothing.
 */
#if (CONFIG_MP_MAX_NUM_CPUS == 1)
#define LOCK_SCHED_SPINLOCK
#else
#define LOCK_SCHED_SPINLOCK   K_SPINLOCK(&_sched_spinlock)
#endif

#endif /* ZEPHYR_KERNEL_INCLUDE_KSPINLOCK_H_ */
