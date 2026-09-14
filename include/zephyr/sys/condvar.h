/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_CONDVAR_H_
#define ZEPHYR_INCLUDE_SYS_CONDVAR_H_

#include <stdint.h>
#include <time.h>

#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup user_condvar_apis User mode condition variable APIs
 * @ingroup usermode_apis
 *
 * A sys_condvar lives in user memory and is used with a @ref sys_mutex.
 * Signalling is an atomic operation on the object itself; the kernel is
 * entered only to block a waiter (k_futex_wait()) or to wake one
 * (k_futex_wake()). Any thread that can write the object's memory can use it.
 * @{
 */

/** @brief Condition variable that lives in user memory */
struct sys_condvar {
	/** @cond INTERNAL_HIDDEN */
	/* the leading words are a struct k_futex: sequence word, kernel-maintained waiter count */
	atomic_t seq;
	atomic_t waiters;
	uint32_t options;
	/** @endcond */
};

/**
 * @brief Statically define and initialize a sys_condvar
 *
 * Absolute timeouts of the condition variable are measured against
 * @ref SYS_CLOCK_REALTIME; see @ref SYS_CONDVAR_DEFINE_EXT for other clocks.
 * Route this to memory domains using K_APP_DMEM().
 *
 * @param name Name of the condition variable.
 */
#define SYS_CONDVAR_DEFINE(name) struct sys_condvar name

/**
 * @brief Statically define and initialize a sys_condvar with options
 *
 * @param name Name of the condition variable.
 * @param opts The clock absolute timeouts are measured against: @ref SYS_CLOCK_REALTIME or
 *             @ref SYS_CLOCK_MONOTONIC.
 */
#define SYS_CONDVAR_DEFINE_EXT(name, opts) struct sys_condvar name = {.options = (opts)}

/**
 * @brief Initialize a condition variable.
 *
 * Absolute timeouts are measured against @ref SYS_CLOCK_REALTIME.
 *
 * @param condvar Address of the condition variable.
 */
void sys_condvar_init(struct sys_condvar *condvar);

/**
 * @brief Initialize a condition variable with options.
 *
 * @param condvar Address of the condition variable.
 * @param options The clock absolute timeouts are measured against: @ref SYS_CLOCK_REALTIME or
 *                @ref SYS_CLOCK_MONOTONIC.
 *
 * @retval 0 Condition variable initialized.
 * @retval -EINVAL Invalid options.
 */
int sys_condvar_init_ext(struct sys_condvar *condvar, uint32_t options);

/**
 * @brief Destroy a condition variable.
 *
 * Waits for threads that were woken but have not yet left the wait, then
 * invalidates the object. The object must be initialized again before any
 * further use.
 *
 * @param condvar Address of the condition variable.
 *
 * @retval 0 Condition variable destroyed.
 */
int sys_condvar_destroy(struct sys_condvar *condvar);

/**
 * @brief Return the clock the condition variable measures absolute timeouts against.
 *
 * @param condvar Address of the condition variable.
 * @return @ref SYS_CLOCK_REALTIME or @ref SYS_CLOCK_MONOTONIC.
 */
uint32_t sys_condvar_clock(const struct sys_condvar *condvar);

/**
 * @brief Signal one thread waiting on the condition variable.
 *
 * @param condvar Address of the condition variable.
 *
 * @retval 0 On success.
 */
int sys_condvar_signal(struct sys_condvar *condvar);

/**
 * @brief Wake every thread waiting on the condition variable.
 *
 * @param condvar Address of the condition variable.
 *
 * @retval 0 On success.
 */
int sys_condvar_broadcast(struct sys_condvar *condvar);

/**
 * @brief Wait on the condition variable, releasing the mutex.
 *
 * The mutex is released while the thread waits and locked again before the
 * function returns, whatever the outcome. Waits can end spuriously, so the
 * caller must re-check its condition.
 *
 * @param condvar Address of the condition variable.
 * @param mutex Address of the mutex, locked by the caller.
 * @param timeout Waiting period, or one of the special values K_NO_WAIT and K_FOREVER.
 *
 * @retval 0 On success or spurious wake-up.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -EPERM Caller does not own @a mutex.
 */
int sys_condvar_wait(struct sys_condvar *condvar, struct sys_mutex *mutex, k_timeout_t timeout);

/**
 * @brief Wait on the condition variable until an absolute time.
 *
 * Like @ref sys_condvar_wait, with @a abstime measured against @a clock.
 *
 * @param condvar Address of the condition variable.
 * @param mutex Address of the mutex, locked by the caller.
 * @param clock @ref SYS_CLOCK_REALTIME or @ref SYS_CLOCK_MONOTONIC.
 * @param abstime Absolute time at which the wait ends.
 *
 * @retval 0 On success or spurious wake-up.
 * @retval -ETIMEDOUT The absolute time has passed.
 * @retval -EINVAL Invalid clock or time.
 * @retval -EPERM Caller does not own @a mutex.
 */
int sys_condvar_clockwait(struct sys_condvar *condvar, struct sys_mutex *mutex, uint32_t clock,
			  const struct timespec *abstime);

/**
 * @brief Wait on the condition variable until an absolute time.
 *
 * Like @ref sys_condvar_clockwait, with @a abstime measured against the clock
 * the condition variable was initialized with.
 *
 * @param condvar Address of the condition variable.
 * @param mutex Address of the mutex, locked by the caller.
 * @param abstime Absolute time at which the wait ends.
 *
 * @retval 0 On success or spurious wake-up.
 * @retval -ETIMEDOUT The absolute time has passed.
 * @retval -EINVAL Invalid time.
 * @retval -EPERM Caller does not own @a mutex.
 */
int sys_condvar_timedwait(struct sys_condvar *condvar, struct sys_mutex *mutex,
			  const struct timespec *abstime);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_CONDVAR_H_ */
