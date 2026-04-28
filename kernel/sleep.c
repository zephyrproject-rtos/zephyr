/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread sleep/wake APIs
 *
 * Implements k_sleep(), k_usleep(), and the internal z_tick_sleep() helper.
 * These APIs suspend the calling thread for a specified duration and rely on
 * the scheduler (via z_sched_unready_locked()) and the timeout subsystem
 * (via z_add_thread_timeout()) to do the heavy lifting.
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <kthread.h>
#include <kswap.h>
#include <timeout_q.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

/* pending_current is owned by timeslicing.c; we reference it here to avoid
 * a spurious timeslice when the thread that is going to sleep gets picked as
 * the next thread to run while z_swap() is in progress.
 */
#if defined(CONFIG_SWAP_NONATOMIC) && defined(CONFIG_TIMESLICING)
extern struct k_thread *pending_current;
#endif

/**
 * @brief Sleep the current thread for the given number of ticks.
 *
 * Removes _current from the run queue, arms a timeout for the requested
 * duration, then context-switches away.  Returns the number of ticks
 * remaining if the sleep was cut short by k_wakeup(), or 0 on normal
 * wakeup.
 *
 * @param timeout Sleep duration.
 * @return Ticks remaining when woken early, or 0 on normal expiry.
 */
static int32_t z_tick_sleep(k_timeout_t timeout)
{
	uint32_t expected_wakeup_ticks;

	__ASSERT(!arch_is_in_isr(), "");

	/* K_NO_WAIT is treated as a 'yield' */
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		k_yield();
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);

#if defined(CONFIG_TIMESLICING) && defined(CONFIG_SWAP_NONATOMIC)
	pending_current = _current;
#endif /* CONFIG_TIMESLICING && CONFIG_SWAP_NONATOMIC */
	z_sched_unready_locked(_current);
	expected_wakeup_ticks = (uint32_t)z_add_thread_timeout(_current, timeout);
	z_mark_thread_as_sleeping(_current);

	(void)z_swap(&_sched_spinlock, key);

	if (!z_is_aborted_thread_timeout(_current)) {
		return 0;
	}

	/* We require a 32 bit unsigned subtraction to handle a wraparound */
	uint32_t left_ticks = expected_wakeup_ticks - sys_clock_tick_get_32();

	/* Use signed comparison so past-due wakeups (negative remainder) return 0.
	 * k_ticks_t may be uint32_t (!CONFIG_TIMEOUT_64BIT), so comparing ticks > 0
	 * directly would be an unsigned comparison and would misinterpret a negative
	 * remainder as a large positive value.
	 */
	int32_t signed_left = (int32_t)left_ticks;

	if (signed_left > 0) {
		return (k_ticks_t)signed_left;
	}

	return 0;
}

int32_t z_impl_k_sleep(k_timeout_t timeout)
{
	k_ticks_t ticks;

	__ASSERT(!arch_is_in_isr(), "");

	SYS_PORT_TRACING_FUNC_ENTER(k_thread, sleep, timeout);

	ticks = z_tick_sleep(timeout);

	/* k_sleep() still returns 32 bit milliseconds for compatibility */
	int64_t ms = K_TIMEOUT_EQ(timeout, K_FOREVER) ? K_TICKS_FOREVER :
		clamp(k_ticks_to_ms_ceil64(ticks), 0, INT_MAX);

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, sleep, timeout, ms);
	return (int32_t) ms;
}

#ifdef CONFIG_USERSPACE
static inline int32_t z_vrfy_k_sleep(k_timeout_t timeout)
{
	return z_impl_k_sleep(timeout);
}
#include <zephyr/syscalls/k_sleep_mrsh.c>
#endif /* CONFIG_USERSPACE */

int32_t z_impl_k_usleep(int32_t us)
{
	int32_t ticks;

	SYS_PORT_TRACING_FUNC_ENTER(k_thread, usleep, us);

	ticks = k_us_to_ticks_ceil64(us);
	ticks = z_tick_sleep(Z_TIMEOUT_TICKS(ticks));

	int32_t ret = k_ticks_to_us_ceil64(ticks);

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, usleep, us, ret);

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int32_t z_vrfy_k_usleep(int32_t us)
{
	return z_impl_k_usleep(us);
}
#include <zephyr/syscalls/k_usleep_mrsh.c>
#endif /* CONFIG_USERSPACE */
