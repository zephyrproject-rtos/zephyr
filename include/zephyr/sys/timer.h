/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_TIMER_H_
#define ZEPHYR_INCLUDE_SYS_TIMER_H_

#include <zephyr/kernel.h>
#include <zephyr/kernel/signal.h>
#include <zephyr/sys/clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @defgroup sys_timer_apis System Timer Pool APIs
 * @ingroup os_services
 *
 * @brief OS-managed pools of kernel timer objects
 *
 * The @c sys_timer API manages allocation, deallocation, and the advanced initialization of
 * @ref k_timer objects from an OS-owned pool, behind system calls. It is the substrate for
 * POSIX timers (`timer_create()` / `timer_delete()`), but is equally usable by native
 * applications that want create-and-go timers without static definitions.
 *
 * The pool is an elastipool: @c SYS_TIMER_MIN objects (the sum of
 * @kconfig{CONFIG_SYS_TIMER_MIN} and all `CONFIG_SYS_TIMER_MIN_ADD_*` contributions) are
 * statically allocated and guaranteed; up to @kconfig{CONFIG_SYS_TIMER_MAX} objects in total
 * may be allocated, the excess coming from the heap without guarantees. Setting the maximum
 * equal to the minimum prohibits dynamic allocation entirely.
 *
 * Timer arming and queries use the clock-based @ref k_timer_settime and @ref k_timer_gettime
 * APIs (@kconfig{CONFIG_TIMER_CLOCK}), regardless of how the timer was allocated.
 *
 * @{
 */

/**
 * @brief Expiry notification description for sys_timer_create()
 *
 * Describes the signal generated when the timer expires. Requires
 * @kconfig{CONFIG_TIMER_SIGNAL}.
 */
struct k_timer_notify {
	/** Thread the signal is delivered to. */
	k_pid_t target;
	/** Kernel signal number to queue on expiry */
	int signo;
	/** Value accompanying the queued signal (or passed to @ref k_timer_notify.fn) */
	union k_sig_val value;

	/**
	 * Function notification: when non-NULL, each expiry invokes @p fn(value) in a fresh
	 * thread spawned by the system timer dispatcher, and @ref k_timer_notify.target /
	 * @ref k_timer_notify.signo are unused. Expiries occurring while a notification is
	 * pending are accounted as overruns (see k_timer_overrun()), never queued twice.
	 * When the creator is a user thread, notification threads run in user mode, in the
	 * creator's memory domain, with the creator's object permissions at dispatch time.
	 */
	void (*fn)(union k_sig_val value);

	/** Stack size for notification threads; 0 selects @kconfig{CONFIG_SYS_THREAD_STACK_SIZE} */
	size_t fn_stack_size;

	/** Thread priority for notification threads (used only when @ref k_timer_notify.fn is set) */
	int fn_priority;
};

/**
 * @brief sys_timer_create() flag: deliver the timer's own address as the notification value
 *
 * When set, the created timer's address overrides @c notify->value as the value carried by
 * expiry signals (as `sival_ptr`). POSIX `timer_create()` uses this for the `evp == NULL`
 * default, whose notification value is the timer ID - unknowable before creation.
 * Requires @kconfig{CONFIG_TIMER_SIGNAL}.
 */
#define SYS_TIMER_SIGVAL_SELF BIT(0)

#if defined(CONFIG_SYS_TIMER) || defined(__DOXYGEN__)

/**
 * @brief Allocate and initialize a kernel timer from the OS timer pool
 *
 * Allocates a @ref k_timer from the system timer pool, binds it to @a clock_id
 * (@c SYS_CLOCK_MONOTONIC or @c SYS_CLOCK_REALTIME), optionally attaches signal-based expiry
 * notification (@a notify, requires @kconfig{CONFIG_TIMER_SIGNAL}), and - if @a value is
 * non-NULL - arms it atomically at creation, exactly as
 * `k_timer_settime(*timer, flags, value, interval, NULL, NULL)` would.
 *
 * With @a value NULL the timer is created disarmed (POSIX `timer_create()` semantics; arm it
 * later with @ref k_timer_settime).
 *
 * When called from a user thread, the calling thread is granted access to the new timer
 * object.
 *
 * TODO: pool objects become per-process resources, released on process exit.
 *
 * @param clock_id  Clock the timer measures against.
 * @param notify    Optional expiry notification description (NULL for none).
 * @param flags     0, @c SYS_TIMER_ABSTIME (applies to @a value), and/or
 *                  @ref SYS_TIMER_SIGVAL_SELF (applies to @a notify).
 * @param value     Optional initial expiration; NULL to create the timer disarmed.
 * @param interval  Optional period (used only when @a value is non-NULL).
 * @param timer     Output: the allocated timer.
 *
 * @retval 0 on success
 * @retval -EAGAIN when the pool is exhausted
 * @retval -EINVAL on invalid clock, flags, or timespec values
 * @retval -ENOTSUP when @a notify is non-NULL without @kconfig{CONFIG_TIMER_SIGNAL}
 */
__syscall int sys_timer_create(int clock_id, const struct k_timer_notify *notify, uint32_t flags,
			       const struct timespec *value, const struct timespec *interval,
			       struct k_timer **timer);

/**
 * @brief Stop, tear down, and release a kernel timer back to the OS timer pool
 *
 * Stops @a timer, detaches any expiry notification (purging queued-but-undelivered signal
 * instances so that no kernel reference to the timer survives this call), revokes user-mode
 * grants, and returns the object to the pool.
 *
 * @param timer Timer previously allocated with sys_timer_create().
 *
 * @retval 0 on success
 * @retval -EINVAL when @a timer was not allocated from the pool (stale or foreign handle)
 */
__syscall int sys_timer_delete(struct k_timer *timer);

#include <zephyr/syscalls/timer.h>

#endif /* defined(CONFIG_SYS_TIMER) || defined(__DOXYGEN__) */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_TIMER_H_ */
