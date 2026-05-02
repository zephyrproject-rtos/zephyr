/**
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CLEANUP_KERNEL_H_
#define ZEPHYR_INCLUDE_CLEANUP_KERNEL_H_

/**
 * @file
 * @brief cleanup helper macros for kernel functions
 */

#if defined(CONFIG_SCOPE_CLEANUP_HELPERS)

#include <zephyr/cleanup.h>
#include <zephyr/kernel.h>

/**
 * @addtogroup cleanup_interface
 * @{
 */

/**
 * @brief Guard for k_mutex.
 * Usage: @code{.c} scope_guard(k_mutex)(&lock); @endcode
 */
SCOPE_GUARD_DEFINE(k_mutex, struct k_mutex *, (void)k_mutex_lock(_T, K_FOREVER),
		   (void)k_mutex_unlock(_T));
/**
 * @brief Guard for k_sem.
 * Usage: @code{.c} scope_guard(k_sem)(&sem); @endcode
 */
SCOPE_GUARD_DEFINE(k_sem, struct k_sem *, (void)k_sem_take(_T, K_FOREVER), k_sem_give(_T));

/**
 * @brief Defer k_free.
 * Usage: @code{.c} scope_defer(k_free)(ptr); @endcode
 */
SCOPE_DEFER_DEFINE(k_free, void *);

/**
 * @brief Defer k_heap_free.
 * Usage: @code{.c} scope_defer(k_heap_free)(&heap, ptr); @endcode
 */
SCOPE_DEFER_DEFINE(k_heap_free, struct k_heap *, void *);

/**
 * @brief Defer k_mem_slab_free.
 * Usage: @code{.c} scope_defer(k_mem_slab_free)(&slab, ptr); @endcode
 */
SCOPE_DEFER_DEFINE(k_mem_slab_free, struct k_mem_slab *, void *);

/**
 * @brief Defer k_mutex_unlock.
 * Usage: @code{.c} scope_defer(k_mutex_unlock)(&lock); @endcode
 */
SCOPE_DEFER_DEFINE(k_mutex_unlock, struct k_mutex *);

/**
 * @brief Defer k_sched_unlock.
 * Usage: @code{.c} scope_defer(k_sched_unlock)(); @endcode
 */
SCOPE_DEFER_DEFINE(k_sched_unlock);

/**
 * @brief Defer k_sem_give.
 * Usage: @code{.c} scope_defer(k_sem_give)(&sem); @endcode
 */
SCOPE_DEFER_DEFINE(k_sem_give, struct k_sem *);

/**
 * @}
 */

#endif /* defined(CONFIG_SCOPE_CLEANUP_HELPERS) */

#endif /* ZEPHYR_INCLUDE_CLEANUP_KERNEL_H_ */
