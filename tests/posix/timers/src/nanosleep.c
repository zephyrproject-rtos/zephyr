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

void common_lower_bound_check(int selection, clockid_t clock_id, int flags, const uint32_t s,
			      uint32_t ns);
int select_nanosleep(int selection, clockid_t clock_id, int flags, const struct timespec *rqtp,
		     struct timespec *rmtp);

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
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d", (int)rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d", (int)rem.tv_nsec, 0);

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
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d", (int)rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d", (int)rem.tv_nsec, 0);

	/*
	 * req and rem point to the same timespec
	 *
	 * Normative spec says they may be the same.
	 * Expect rem to be zero after returning.
	 */
	req = (struct timespec){.tv_sec = 0, .tv_nsec = 1};
	zassert_equal(select_nanosleep(selection, clock_id, flags, &req, &req), 0);
	zassert_equal(errno, 0);
	zassert_equal(req.tv_sec, 0, "actual: %d expected: %d", (int)req.tv_sec, 0);
	zassert_equal(req.tv_nsec, 0, "actual: %d expected: %d", (int)req.tv_nsec, 0);
}

ZTEST(posix_timers, test_nanosleep_errors_errno)
{
	common_errors(SELECT_NANOSLEEP, CLOCK_REALTIME, 0);
}

ZTEST(posix_timers, test_clock_nanosleep_errors_errno)
{
	struct timespec rem = {};
	struct timespec req = {};

	common_errors(SELECT_CLOCK_NANOSLEEP, CLOCK_MONOTONIC, TIMER_ABSTIME);

	/* Absolute timeout in the past. */
	clock_gettime(CLOCK_MONOTONIC, &req);
	zassert_equal(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &req, &rem), 0);
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d", (int)rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d", (int)rem.tv_nsec, 0);

	/* Absolute timeout in the past relative to the realtime clock. */
	clock_gettime(CLOCK_REALTIME, &req);
	zassert_equal(clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &req, &rem), 0);
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d", (int)rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d", (int)rem.tv_nsec, 0);
}

ZTEST(posix_timers, test_nanosleep_execution)
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
