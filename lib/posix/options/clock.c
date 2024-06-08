/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/posix/time.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/sys/realtime.h>

static bool __posix_clock_validate_timespec(const struct timespec *ts)
{
	return ts->tv_sec >= 0 && ts->tv_nsec >= 0 && ts->tv_nsec < NSEC_PER_SEC;
}

static void __posix_clock_k_ticks_to_timespec(struct timespec *ts, int64_t ticks)
{
	uint64_t elapsed_secs = ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	uint64_t nremainder = ticks - elapsed_secs * CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	ts->tv_sec = (time_t) elapsed_secs;
	/* For ns 32 bit conversion can be used since its smaller than 1sec. */
	ts->tv_nsec = (int32_t) k_ticks_to_ns_floor32(nremainder);
}

static void __posix_clock_get_monotonic(struct timespec *ts)
{
	__posix_clock_k_ticks_to_timespec(ts, k_uptime_ticks());
}

static void __posix_clock_msec_to_timespec(struct timespec *ts, const int64_t *ms)
{
	ts->tv_sec = *ms / MSEC_PER_SEC;
	ts->tv_nsec = (*ms % MSEC_PER_SEC) * NSEC_PER_MSEC;
}

static uint64_t __posix_clock_timespec_to_usec(const struct timespec *ts)
{
	uint64_t usec;

	usec = ts->tv_sec;
	usec *= USEC_PER_SEC;
	usec += DIV_ROUND_UP(ts->tv_nsec, NSEC_PER_USEC);
	return usec;
}

static uint64_t __posix_clock_timespec_to_msec(const struct timespec *ts)
{
	uint64_t msec;

	msec = ts->tv_sec;
	msec *= MSEC_PER_SEC;
	msec += DIV_ROUND_UP(ts->tv_nsec, NSEC_PER_MSEC);
	return msec;
}

/* Check if a_ts is less than b_ts (a_ts < b_ts) */
static bool __posix_clock_timespec_less_than(const struct timespec *a_ts,
					     const struct timespec *b_ts)
{
	return (a_ts->tv_sec < b_ts->tv_sec) ||
	       (a_ts->tv_sec == b_ts->tv_sec && a_ts->tv_nsec < b_ts->tv_nsec);
}

/*
 * Subtract b_ts from a_ts placing result in res_ts (ret_ts = a_ts - b_ts)
 * Presumes a_ts >= b_ts
 */
static void __posix_clock_timespec_subtract(struct timespec *res_ts,
					    const struct timespec *a_ts,
					    const struct timespec *b_ts)
{
	res_ts->tv_sec = a_ts->tv_sec - b_ts->tv_sec;

	if (b_ts->tv_nsec <= a_ts->tv_nsec) {
		res_ts->tv_nsec = a_ts->tv_nsec - b_ts->tv_nsec;
	} else {
		res_ts->tv_sec--;
		res_ts->tv_nsec = a_ts->tv_nsec + NSEC_PER_SEC - b_ts->tv_nsec;
	}
}

/* Add b_ts to a_ts placing result in res_ts (ret_ts = a_ts + b_ts) */
static void __posix_clock_timespec_add(struct timespec *res_ts,
				       const struct timespec *a_ts,
				       const struct timespec *b_ts)
{
	res_ts->tv_sec = a_ts->tv_sec + b_ts->tv_sec;
	res_ts->tv_nsec = a_ts->tv_nsec + b_ts->tv_nsec;

	if (res_ts->tv_nsec >= NSEC_PER_SEC) {
		res_ts->tv_sec++;
		res_ts->tv_nsec -= NSEC_PER_SEC;
	}
}

static void __posix_clock_timespec_copy(struct timespec *des_ts, const struct timespec *src_ts)
{
	des_ts->tv_sec = src_ts->tv_sec;
	des_ts->tv_nsec = src_ts->tv_nsec;
}

static void __posix_clock_get_realtime(struct timespec *ts)
{
	int res;
	int64_t timestamp_ms;

	res = sys_realtime_get_timestamp(&timestamp_ms);
	if (timestamp_ms < 0) {
		/* timespec can't be negative */
		ts->tv_sec = 0;
		ts->tv_nsec = 0;
	}

	__posix_clock_msec_to_timespec(ts, &timestamp_ms);
}

static int __posix_clock_set_realtime(const struct timespec *ts)
{
	int64_t timestamp_ms;
	int res;

	timestamp_ms = (int64_t)__posix_clock_timespec_to_msec(ts);

	res = sys_realtime_set_timestamp(&timestamp_ms);
	if (res < 0) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

int clock_gettime(clockid_t clock_id, struct timespec *ts)
{
	int res;

	switch (clock_id) {
	case CLOCK_MONOTONIC:
		__posix_clock_get_monotonic(ts);
		res = 0;
		break;

	case CLOCK_REALTIME:
		__posix_clock_get_realtime(ts);
		res = 0;
		break;

	default:
		errno = EINVAL;
		res = -1;
	}

	return res;
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
	if (clock_id != CLOCK_REALTIME || !__posix_clock_validate_timespec(tp)) {
		errno = EINVAL;
		return -1;
	}

	return __posix_clock_set_realtime(tp);
}

/*
 * Note: usleep() was removed in Issue 7.
 *
 * It is kept here for compatibility purposes.
 *
 * For more information, please see
 * https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_xsh_chap01.html
 * https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_xsh_chap03.html
 */
int usleep(useconds_t useconds)
{
	int32_t rem;

	if (useconds >= USEC_PER_SEC) {
		errno = EINVAL;
		return -1;
	}

	rem = k_usleep(useconds);
	__ASSERT_NO_MSG(rem >= 0);
	if (rem > 0) {
		/* sleep was interrupted by a call to k_wakeup() */
		errno = EINTR;
		return -1;
	}

	return 0;
}

/**
 * @brief Suspend execution for a nanosecond interval, or
 * until some absolute time relative to the specified clock.
 *
 * See IEEE 1003.1
 */
static int __z_clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *rqtp,
			       struct timespec *rmtp)
{
	volatile uint64_t usec;
	struct timespec clock_ts;
	struct timespec rel_ts;
	struct timespec abs_ts;
	const bool update_rmtp = rmtp != NULL;

	if (!(clock_id == CLOCK_REALTIME || clock_id == CLOCK_MONOTONIC)) {
		errno = EINVAL;
		return -1;
	}

	if (rqtp == NULL) {
		errno = EFAULT;
		return -1;
	}

	if (!__posix_clock_validate_timespec(rqtp)) {
		errno = EINVAL;
		return -1;
	}

	if ((flags & TIMER_ABSTIME) && clock_id == CLOCK_REALTIME) {
		__posix_clock_get_realtime(&clock_ts);

		if (__posix_clock_timespec_less_than(rqtp, &clock_ts)) {
			goto post_sleep;
		}

		__posix_clock_timespec_subtract(&rel_ts, rqtp, &clock_ts);
		__posix_clock_get_monotonic(&clock_ts);
		__posix_clock_timespec_add(&abs_ts, &rel_ts, &clock_ts);
	} else if (flags & TIMER_ABSTIME) {
		__posix_clock_timespec_copy(&abs_ts, rqtp);
	} else {
		__posix_clock_get_monotonic(&clock_ts);
		__posix_clock_timespec_add(&abs_ts, rqtp, &clock_ts);
	}

	usec = __posix_clock_timespec_to_usec(&abs_ts);

	do {
		usec = k_sleep(K_TIMEOUT_ABS_US(usec)) * USEC_PER_MSEC;
	} while (usec != 0);

post_sleep:
	if (update_rmtp) {
		rmtp->tv_sec = 0;
		rmtp->tv_nsec = 0;
	}

	return 0;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	return __z_clock_nanosleep(CLOCK_MONOTONIC, 0, rqtp, rmtp);
}

int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *rqtp,
		    struct timespec *rmtp)
{
	return __z_clock_nanosleep(clock_id, flags, rqtp, rmtp);
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
