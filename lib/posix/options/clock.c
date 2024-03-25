/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_clock.h"

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/posix/time.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/spinlock.h>

/*
 * `k_uptime_get` returns a timestamp based on an always increasing
 * value from the system start.  To support the `CLOCK_REALTIME`
 * clock, this `rt_clock_base` records the time that the system was
 * started.  This can either be set via 'clock_settime', or could be
 * set from a real time clock, if such hardware is present.
 */
static struct timespec rt_clock_base;
static struct k_spinlock rt_clock_base_lock;

/**
 * @brief Get clock time specified by clock_id.
 *
 * See IEEE 1003.1
 */
int z_impl___posix_clock_get_base(clockid_t clock_id, struct timespec *base)
{
	switch (clock_id) {
	case CLOCK_MONOTONIC:
		base->tv_sec = 0;
		base->tv_nsec = 0;
		break;

	case CLOCK_REALTIME:
		K_SPINLOCK(&rt_clock_base_lock) {
			*base = rt_clock_base;
		}
		break;

	default:
		errno = EINVAL;
		return -1;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy___posix_clock_get_base(clockid_t clock_id, struct timespec *ts)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(ts, sizeof(*ts)));
	return z_impl___posix_clock_get_base(clock_id, ts);
}
#include <syscalls/__posix_clock_get_base_mrsh.c>
#endif

int clock_gettime(clockid_t clock_id, struct timespec *ts)
{
	struct timespec base;

	switch (clock_id) {
	case CLOCK_MONOTONIC:
		base.tv_sec = 0;
		base.tv_nsec = 0;
		break;

	case CLOCK_REALTIME:
		(void)__posix_clock_get_base(clock_id, &base);
		break;

	default:
		errno = EINVAL;
		return -1;
	}

	uint64_t ticks = k_uptime_ticks();
	uint64_t elapsed_secs = ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	uint64_t nremainder = ticks - elapsed_secs * CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	ts->tv_sec = (time_t) elapsed_secs;
	/* For ns 32 bit conversion can be used since its smaller than 1sec. */
	ts->tv_nsec = (int32_t) k_ticks_to_ns_floor32(nremainder);

	ts->tv_sec += base.tv_sec;
	ts->tv_nsec += base.tv_nsec;
	if (ts->tv_nsec >= NSEC_PER_SEC) {
		ts->tv_sec++;
		ts->tv_nsec -= NSEC_PER_SEC;
	}

	return 0;
}

int clock_getres(clockid_t clock_id, struct timespec *res)
{
	BUILD_ASSERT(CONFIG_SYS_CLOCK_TICKS_PER_SEC > 0 &&
			     CONFIG_SYS_CLOCK_TICKS_PER_SEC <= NSEC_PER_SEC,
		     "CONFIG_SYS_CLOCK_TICKS_PER_SEC must be > 0 and <= NSEC_PER_SEC");

	if (!(clock_id == CLOCK_MONOTONIC || clock_id == CLOCK_REALTIME ||
	      clock_id == CLOCK_PROCESS_CPUTIME_ID)) {
		errno = EINVAL;
		return -1;
	}

	if (res != NULL) {
		*res = (struct timespec){
			.tv_sec = 0,
			.tv_nsec = NSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC,
		};
	}

	return 0;
}

/**
 * @brief Set the time of the specified clock.
 *
 * See IEEE 1003.1.
 *
 * Note that only the `CLOCK_REALTIME` clock can be set using this
 * call.
 */
int clock_settime(clockid_t clock_id, const struct timespec *tp)
{
	struct timespec base;
	k_spinlock_key_t key;

	if (clock_id != CLOCK_REALTIME) {
		errno = EINVAL;
		return -1;
	}

	if (tp->tv_nsec < 0 || tp->tv_nsec >= NSEC_PER_SEC) {
		errno = EINVAL;
		return -1;
	}

	uint64_t elapsed_nsecs = k_ticks_to_ns_floor64(k_uptime_ticks());
	int64_t delta = (int64_t)NSEC_PER_SEC * tp->tv_sec + tp->tv_nsec
		- elapsed_nsecs;

	base.tv_sec = delta / NSEC_PER_SEC;
	base.tv_nsec = delta % NSEC_PER_SEC;

	key = k_spin_lock(&rt_clock_base_lock);
	rt_clock_base = base;
	k_spin_unlock(&rt_clock_base_lock, key);

	return 0;
}

/**
 * @brief Suspend execution for a nanosecond interval, or
 * until some absolute time relative to the specified clock.
 *
 * See IEEE 1003.1
 */
int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *rqtp,
		    struct timespec *rmtp)
{
	uint64_t ns;
	uint64_t us;
	uint64_t uptime_ns;
	k_spinlock_key_t key;
	const bool update_rmtp = rmtp != NULL;

	if (!(clock_id == CLOCK_REALTIME || clock_id == CLOCK_MONOTONIC)) {
		errno = EINVAL;
		return -1;
	}

	if (rqtp == NULL) {
		errno = EFAULT;
		return -1;
	}

	if (rqtp->tv_sec < 0 || rqtp->tv_nsec < 0 || rqtp->tv_nsec >= NSEC_PER_SEC) {
		errno = EINVAL;
		return -1;
	}

	if ((flags & TIMER_ABSTIME) == 0 &&
	    unlikely(rqtp->tv_sec >= ULLONG_MAX / NSEC_PER_SEC)) {

		ns = rqtp->tv_nsec + NSEC_PER_SEC
			+ k_sleep(K_SECONDS(rqtp->tv_sec - 1)) * NSEC_PER_MSEC;
	} else {
		ns = rqtp->tv_sec * NSEC_PER_SEC + rqtp->tv_nsec;
	}

	uptime_ns = k_cyc_to_ns_ceil64(k_cycle_get_32());

	if (flags & TIMER_ABSTIME && clock_id == CLOCK_REALTIME) {
		key = k_spin_lock(&rt_clock_base_lock);
		ns -= rt_clock_base.tv_sec * NSEC_PER_SEC + rt_clock_base.tv_nsec;
		k_spin_unlock(&rt_clock_base_lock, key);
	}

	if ((flags & TIMER_ABSTIME) == 0) {
		ns += uptime_ns;
	}

	if (ns <= uptime_ns) {
		goto do_rmtp_update;
	}

	us = DIV_ROUND_UP(ns, NSEC_PER_USEC);
	do {
		us = k_sleep(K_TIMEOUT_ABS_US(us)) * 1000;
	} while (us != 0);

do_rmtp_update:
	if (update_rmtp) {
		rmtp->tv_sec = 0;
		rmtp->tv_nsec = 0;
	}

	return 0;
}

/**
 * @brief Get current real time.
 *
 * See IEEE 1003.1
 */
int gettimeofday(struct timeval *tv, void *tz)
{
	struct timespec ts;
	int res;

	/* As per POSIX, "if tzp is not a null pointer, the behavior
	 * is unspecified."  "tzp" is the "tz" parameter above. */
	ARG_UNUSED(tz);

	res = clock_gettime(CLOCK_REALTIME, &ts);
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / NSEC_PER_USEC;

	return res;
}

int clock_getcpuclockid(pid_t pid, clockid_t *clock_id)
{
	/* We don't allow any process ID but our own.  */
	if (pid != 0 && pid != getpid()) {
		return EPERM;
	}

	*clock_id = CLOCK_PROCESS_CPUTIME_ID;

	return 0;
}

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
static void reset_clock_base(void)
{
	K_SPINLOCK(&rt_clock_base_lock) {
		rt_clock_base = (struct timespec){0};
	}
}

static void clock_base_reset_rule_after(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);

	reset_clock_base();
}

ZTEST_RULE(clock_base_reset_rule, NULL, clock_base_reset_rule_after);
#endif /* CONFIG_ZTEST */
