/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/condvar.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/sys/timeutil.h>

BUILD_ASSERT(offsetof(struct sys_condvar, seq) == offsetof(struct k_futex, val));
BUILD_ASSERT(offsetof(struct sys_condvar, waiters) == offsetof(struct k_futex, waiters));

static inline struct k_futex *condvar_futex(struct sys_condvar *condvar)
{
	return (struct k_futex *)&condvar->seq;
}

static inline void condvar_seq_bump(struct sys_condvar *condvar)
{
	(void)atomic_inc(&condvar->seq);
}

void sys_condvar_init(struct sys_condvar *condvar)
{
	(void)sys_condvar_init_ext(condvar, SYS_CLOCK_REALTIME);
}

int sys_condvar_init_ext(struct sys_condvar *condvar, uint32_t options)
{
	switch (options) {
	case SYS_CLOCK_REALTIME:
	case SYS_CLOCK_MONOTONIC:
		break;
	default:
		return -EINVAL;
	}

	*condvar = (struct sys_condvar){.options = options};

	return 0;
}

int sys_condvar_destroy(struct sys_condvar *condvar)
{
	/* woken waiters are already uncounted; a timed-out one uncounts itself before returning */
	while (atomic_get(&condvar->waiters) != 0) {
		k_sleep(K_TICKS(1));
	}

	*condvar = (struct sys_condvar){0};

	return 0;
}

uint32_t sys_condvar_clock(const struct sys_condvar *condvar)
{
	/* a zero-filled object is a valid condition variable on the default clock */
	return (condvar->options == 0) ? SYS_CLOCK_REALTIME : condvar->options;
}

/*
 * A waiter that has released the mutex but not yet blocked is caught by the
 * sequence compare in k_futex_wait(); one that has blocked is counted by the
 * kernel. Waking a blocked waiter without moving the sequence keeps a signal
 * to one thread; the sequence only moves when no blocked waiter was found.
 */
int sys_condvar_signal(struct sys_condvar *condvar)
{
	if ((atomic_get(&condvar->waiters) != 0) &&
	    (k_futex_wake(condvar_futex(condvar), false) > 0)) {
		return 0;
	}

	condvar_seq_bump(condvar);
	if (atomic_get(&condvar->waiters) != 0) {
		(void)k_futex_wake(condvar_futex(condvar), false);
	}

	return 0;
}

int sys_condvar_broadcast(struct sys_condvar *condvar)
{
	condvar_seq_bump(condvar);
	if (atomic_get(&condvar->waiters) != 0) {
		(void)k_futex_wake(condvar_futex(condvar), true);
	}

	return 0;
}

int sys_condvar_wait(struct sys_condvar *condvar, struct sys_mutex *mutex, k_timeout_t timeout)
{
	int ret;
	atomic_val_t seq;

	seq = atomic_get(&condvar->seq);

	ret = sys_mutex_unlock(mutex);
	if (ret < 0) {
		return ret;
	}

	ret = k_futex_wait(condvar_futex(condvar), seq, timeout);
	(void)sys_mutex_lock(mutex, K_FOREVER);

	return (ret == -ETIMEDOUT) ? -EAGAIN : 0;
}

int sys_condvar_clockwait(struct sys_condvar *condvar, struct sys_mutex *mutex, uint32_t clock,
			  const struct timespec *abstime)
{
	int ret;
	k_timepoint_t timepoint;

	switch (clock) {
	case SYS_CLOCK_REALTIME:
	case SYS_CLOCK_MONOTONIC:
		break;
	default:
		return -EINVAL;
	}

	if ((abstime == NULL) || !timespec_is_valid(abstime)) {
		return -EINVAL;
	}

	timepoint = timespec_abs_to_timepoint(clock, abstime);
	if (sys_timepoint_expired(timepoint)) {
		return -ETIMEDOUT;
	}

	ret = sys_condvar_wait(condvar, mutex, sys_timepoint_timeout(timepoint));

	return (ret == -EAGAIN) ? -ETIMEDOUT : ret;
}

int sys_condvar_timedwait(struct sys_condvar *condvar, struct sys_mutex *mutex,
			  const struct timespec *abstime)
{
	return sys_condvar_clockwait(condvar, mutex, sys_condvar_clock(condvar), abstime);
}
