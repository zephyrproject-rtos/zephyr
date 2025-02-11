/*
 * Copyright (c) 2018 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <time.h>

#include <zephyr/sys_clock.h>
#include <zephyr/ztest.h>

#define SELECT_NANOSLEEP       1
#define SELECT_CLOCK_NANOSLEEP 0

static inline int select_nanosleep(int selection, clockid_t clock_id, int flags,
				   const struct timespec *rqtp, struct timespec *rmtp)
{
	if (selection == SELECT_NANOSLEEP) {
		return nanosleep(rqtp, rmtp);
	}
	return clock_nanosleep(clock_id, flags, rqtp, rmtp);
}

static inline uint64_t cycle_get_64(void)
{
	if (IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		return k_cycle_get_64();
	} else {
		return k_cycle_get_32();
	}
}

static void common_errors(int selection, clockid_t clock_id, int flags)
{
	struct timespec rem = {};
	struct timespec req = {};

	/*
	 * invalid parameters
	 */
	zassert_equal(select_nanosleep(selection, clock_id, flags, NULL, NULL), -1);
	zassert_equal(errno, EFAULT);

	/* NULL request */
	errno = 0;
	zassert_equal(select_nanosleep(selection, clock_id, flags, NULL, &rem), -1);
	zassert_equal(errno, EFAULT);
	/* Expect rem to be the same when function returns */
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d", rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d", rem.tv_nsec, 0);

	/* negative times */
	errno = 0;
	req = (struct timespec){.tv_sec = -1, .tv_nsec = 0};
	zassert_equal(select_nanosleep(selection, clock_id, flags, &req, NULL), -1);
	zassert_equal(errno, EINVAL);

	errno = 0;
	req = (struct timespec){.tv_sec = 0, .tv_nsec = -1};
	zassert_equal(select_nanosleep(selection, clock_id, flags, &req, NULL), -1);
	zassert_equal(errno, EINVAL);

	errno = 0;
	req = (struct timespec){.tv_sec = -1, .tv_nsec = -1};
	zassert_equal(select_nanosleep(selection, clock_id, flags, &req, NULL), -1);
	zassert_equal(errno, EINVAL);

	/* nanoseconds too high */
	errno = 0;
	req = (struct timespec){.tv_sec = 0, .tv_nsec = 1000000000};
	zassert_equal(select_nanosleep(selection, clock_id, flags, &req, NULL), -1);
	zassert_equal(errno, EINVAL);

	/*
	 * Valid parameters
	 */
	errno = 0;

	/* Happy path, plus make sure the const input is unmodified */
	req = (struct timespec){.tv_sec = 1, .tv_nsec = 1};
	zassert_equal(select_nanosleep(selection, clock_id, flags, &req, NULL), 0);
	zassert_equal(errno, 0);
	zassert_equal(req.tv_sec, 1);
	zassert_equal(req.tv_nsec, 1);

	/* Sleep for 0.0 s. Expect req & rem to be the same when function returns */
	zassert_equal(select_nanosleep(selection, clock_id, flags, &req, &rem), 0);
	zassert_equal(errno, 0);
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d", rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d", rem.tv_nsec, 0);

	/*
	 * req and rem point to the same timespec
	 *
	 * Normative spec says they may be the same.
	 * Expect rem to be zero after returning.
	 */
	req = (struct timespec){.tv_sec = 0, .tv_nsec = 1};
	zassert_equal(select_nanosleep(selection, clock_id, flags, &req, &req), 0);
	zassert_equal(errno, 0);
	zassert_equal(req.tv_sec, 0, "actual: %d expected: %d", req.tv_sec, 0);
	zassert_equal(req.tv_nsec, 0, "actual: %d expected: %d", req.tv_nsec, 0);
}

ZTEST(nanosleep, test_nanosleep_errors_errno)
{
	common_errors(SELECT_NANOSLEEP, CLOCK_REALTIME, 0);
}

ZTEST(nanosleep, test_clock_nanosleep_errors_errno)
{
	struct timespec rem = {};
	struct timespec req = {};

	common_errors(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME);

	/* Absolute timeout in the past. */
	clock_gettime(CLOCK_MONOTONIC, &req);
	zassert_equal(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &req, &rem), 0);
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d", rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d", rem.tv_nsec, 0);

	/* Absolute timeout in the past relative to the realtime clock. */
	clock_gettime(CLOCK_REALTIME, &req);
	zassert_equal(clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &req, &rem), 0);
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d", rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d", rem.tv_nsec, 0);
}

/**
 * @brief Check that a call to nanosleep has yielded execution for some minimum time.
 *
 * Check that the actual time slept is >= the total time specified by @p s (in seconds) and
 * @p ns (in nanoseconds).
 *
 * @note The time specified by @p s and @p ns is assumed to be absolute (i.e. a time-point)
 * when @p selection is set to @ref SELECT_CLOCK_NANOSLEEP. The time is assumed to be relative
 * when @p selection is set to @ref SELECT_NANOSLEEP.
 *
 * @param selection Either @ref SELECT_CLOCK_NANOSLEEP or @ref SELECT_NANOSLEEP
 * @param clock_id The clock to test (e.g. @ref CLOCK_MONOTONIC or @ref CLOCK_REALTIME)
 * @param flags Flags to pass to @ref clock_nanosleep
 * @param s Partial lower bound for yielded time (in seconds)
 * @param ns Partial lower bound for yielded time (in nanoseconds)
 */
static void common_lower_bound_check(int selection, clockid_t clock_id, int flags, const uint32_t s,
				     uint32_t ns)
{
	int r;
	uint64_t actual_ns;
	uint64_t exp_ns;
	uint64_t now;
	uint64_t then;
	struct timespec rem = {0, 0};
	struct timespec req = {s, ns};

	errno = 0;
	then = cycle_get_64();
	r = select_nanosleep(selection, clock_id, flags, &req, &rem);
	now = cycle_get_64();

	zassert_equal(r, 0, "actual: %d expected: %d", r, 0);
	zassert_equal(errno, 0, "actual: %d expected: %d", errno, 0);
	zassert_equal(req.tv_sec, s, "actual: %d expected: %d", req.tv_sec, s);
	zassert_equal(req.tv_nsec, ns, "actual: %d expected: %d", req.tv_nsec, ns);
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d", rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d", rem.tv_nsec, 0);

	switch (selection) {
	case SELECT_NANOSLEEP:
		/* exp_ns and actual_ns are relative (i.e. durations) */
		actual_ns = k_cyc_to_ns_ceil64(now + then);
		break;
	case SELECT_CLOCK_NANOSLEEP:
		/* exp_ns and actual_ns are absolute (i.e. time-points) */
		actual_ns = k_cyc_to_ns_ceil64(now);
		break;
	default:
		zassert_unreachable();
		break;
	}

	exp_ns = (uint64_t)s * NSEC_PER_SEC + ns;
	/* round up to the nearest microsecond for k_busy_wait() */
	exp_ns = DIV_ROUND_UP(exp_ns, NSEC_PER_USEC) * NSEC_PER_USEC;

	/* The comparison may be incorrect if counter wrap happened. In case of ARC HSDK platforms
	 * we have high counter clock frequency (500MHz or 1GHz) so counter wrap quite likely to
	 * happen if we wait long enough. As in some test cases we wait more than 1 second, there
	 * are significant chances to get false-positive assertion.
	 * TODO: switch test for k_cycle_get_64 usage where available.
	 */
#if !defined(CONFIG_SOC_ARC_HSDK) && !defined(CONFIG_SOC_ARC_HSDK4XD)
	/* lower bounds check */
	zassert_true(actual_ns >= exp_ns, "actual: %llu expected: %llu", actual_ns, exp_ns);
#endif

	/* TODO: Upper bounds check when hr timers are available */
}

ZTEST(nanosleep, test_nanosleep_execution)
{
	/* sleep for 1ns */
	common_lower_bound_check(SELECT_NANOSLEEP, 0, 0, 0, 1);

	/* sleep for 1us + 1ns */
	common_lower_bound_check(SELECT_NANOSLEEP, 0, 0, 0, 1001);

	/* sleep for 500000000ns */
	common_lower_bound_check(SELECT_NANOSLEEP, 0, 0, 0, 500000000);

	/* sleep for 1s */
	common_lower_bound_check(SELECT_NANOSLEEP, 0, 0, 1, 0);

	/* sleep for 1s + 1ns */
	common_lower_bound_check(SELECT_NANOSLEEP, 0, 0, 1, 1);

	/* sleep for 1s + 1us + 1ns */
	common_lower_bound_check(SELECT_NANOSLEEP, 0, 0, 1, 1001);
}

ZTEST(nanosleep, test_clock_nanosleep_execution)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	/* absolute sleeps with the monotonic clock and reference time ts */

	/* until 1s + 1ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
		ts.tv_sec + 1, 1);

	/* until 1s + 1us past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
		ts.tv_sec + 1, 1000);

	/* until 1s + 500000000ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
		ts.tv_sec + 1, 500000000);

	/* until 2s past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
		ts.tv_sec + 2, 0);

	/* until 2s + 1ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
		ts.tv_sec + 2, 1);

	/* until 2s + 1us + 1ns past reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME,
		ts.tv_sec + 2, 1001);

	clock_gettime(CLOCK_REALTIME, &ts);

	/* absolute sleeps with the real time clock and adjusted reference time ts */

	/* until 1s + 1ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 1, 1);

	/* until 1s + 1us past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 1, 1000);

	/* until 1s + 500000000ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 1, 500000000);

	/* until 2s past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 2, 0);

	/* until 2s + 1ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 2, 1);

	/* until 2s + 1us + 1ns past the reference time */
	common_lower_bound_check(SELECT_CLOCK_NANOSLEEP, CLOCK_REALTIME, TIMER_ABSTIME,
				 ts.tv_sec + 2, 1001);
}

ZTEST_SUITE(nanosleep, NULL, NULL, NULL, NULL, NULL);
