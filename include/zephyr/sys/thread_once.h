/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_THREAD_ONCE_H_
#define ZEPHYR_INCLUDE_SYS_THREAD_ONCE_H_

#include <zephyr/sys/atomic_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup sys_thread_apis
 * @{
 */

/**
 * @brief One-time initialization control
 *
 * Used with @ref sys_thread_once to run a function exactly once, however many threads race
 * to call it. Initialize with @ref SYS_THREAD_ONCE_INIT or by zero-filling. The object must
 * outlive every caller and, under @kconfig{CONFIG_USERSPACE}, be writable by each of them.
 */
/* a struct k_futex: state word, kernel-maintained waiter count */
typedef struct {
	atomic_t val;
	atomic_t waiters;
} sys_thread_once_t;

/** @brief Static initializer for a @ref sys_thread_once_t */
#define SYS_THREAD_ONCE_INIT {0}

/**
 * @brief Execute a function exactly once.
 *
 * The first caller runs @p func. Concurrent callers block until it returns, and every later
 * call returns at once. If the thread running @p func is cancelled, the next caller runs
 * @p func again. This function is not itself a cancellation point.
 *
 * @param flag Pointer to an initialised @ref sys_thread_once_t.
 * @param func Function to call on the first invocation.
 */
void sys_thread_once(sys_thread_once_t *flag, void (*func)(void));

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_THREAD_ONCE_H_ */
