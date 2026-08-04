/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread sleep/wake APIs
 *
 * Implements k_sleep_ticks(), the primitive that k_sleep(), k_msleep() and
 * k_usleep() are built upon.  It suspends the calling thread for a specified
 * duration and relies on the scheduler (via z_sched_unready_locked()) and the
 * timeout subsystem (via z_add_thread_timeout()) to do the heavy lifting.  The
 * millisecond and microsecond flavours are inlined in sleep.h on top of it.
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
#include <zephyr/sys/minmax.h>
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
 * @return Ticks remaining when woken early, 0 on normal expiry, or
 *         K_TICKS_FOREVER if @a timeout was K_FOREVER.
 */
k_ticks_t z_impl_k_sleep_ticks(k_timeout_t timeout)
{
	uint32_t expected_wakeup_ticks;

	__ASSERT(!arch_is_in_isr(), "");

	SYS_PORT_TRACING_FUNC_ENTER(k_thread, sleep, timeout);

	/* K_NO_WAIT is treated as a 'yield' */
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		k_yield();
		SYS_PORT_TRACING_FUNC_EXIT(k_thread, sleep, timeout, 0);
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

	/* There is no meaningful remainder to report for K_FOREVER: reaching
	 * this point at all means a k_wakeup() cut the sleep short.
	 */
	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		SYS_PORT_TRACING_FUNC_EXIT(k_thread, sleep, timeout, K_TICKS_FOREVER);
		return K_TICKS_FOREVER;
	}

	/* We require a 32 bit unsigned subtraction to handle a wraparound.
	 * A normal timeout-driven wakeup leaves zero (or a slightly negative)
	 * remainder; only an early k_wakeup() yields a positive value.
	 */
	uint32_t left_ticks = expected_wakeup_ticks - sys_clock_tick_get_32();

	/* Use signed comparison so past-due wakeups (negative remainder) return 0.
	 * k_ticks_t may be uint32_t (!CONFIG_TIMEOUT_64BIT), so comparing ticks > 0
	 * directly would be an unsigned comparison and would misinterpret a negative
	 * remainder as a large positive value.
	 */
	int32_t signed_left = (int32_t)left_ticks;

	if (signed_left < 0) {
		signed_left = 0;
	}

	SYS_PORT_TRACING_FUNC_EXIT(k_thread, sleep, timeout, signed_left);
	return (k_ticks_t)signed_left;
}

#ifdef CONFIG_USERSPACE
static inline k_ticks_t z_vrfy_k_sleep_ticks(k_timeout_t timeout)
{
	return z_impl_k_sleep_ticks(timeout);
}
#include <zephyr/syscalls/k_sleep_ticks_mrsh.c>
#endif /* CONFIG_USERSPACE */
