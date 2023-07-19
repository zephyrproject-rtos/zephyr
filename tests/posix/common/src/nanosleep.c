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

ZTEST(posix_apis, test_nanosleep_errors_errno)
{
	struct timespec rem = {};
	struct timespec req = {};

	/*
	 * invalid parameters
	 */
	zassert_equal(nanosleep(NULL, NULL), -1);
	zassert_equal(errno, EFAULT);

	/* NULL request */
	errno = 0;
	zassert_equal(nanosleep(NULL, &rem), -1);
	zassert_equal(errno, EFAULT);
	/* Expect rem to be the same when function returns */
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d", rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d", rem.tv_nsec, 0);

	/* negative times */
	errno = 0;
	req = (struct timespec){.tv_sec = -1, .tv_nsec = 0};
	zassert_equal(nanosleep(&req, NULL), -1);
	zassert_equal(errno, EINVAL);

	errno = 0;
	req = (struct timespec){.tv_sec = 0, .tv_nsec = -1};
	zassert_equal(nanosleep(&req, NULL), -1);
	zassert_equal(errno, EINVAL);

	errno = 0;
	req = (struct timespec){.tv_sec = -1, .tv_nsec = -1};
	zassert_equal(nanosleep(&req, NULL), -1);
	zassert_equal(errno, EINVAL);

	/* nanoseconds too high */
	errno = 0;
	req = (struct timespec){.tv_sec = 0, .tv_nsec = 1000000000};
	zassert_equal(nanosleep(&req, NULL), -1);
	zassert_equal(errno, EINVAL);

	/*
	 * Valid parameters
	 */
	errno = 0;

	/* Happy path, plus make sure the const input is unmodified */
	req = (struct timespec){.tv_sec = 1, .tv_nsec = 1};
	zassert_equal(nanosleep(&req, NULL), 0);
	zassert_equal(errno, 0);
	zassert_equal(req.tv_sec, 1);
	zassert_equal(req.tv_nsec, 1);

	/* Sleep for 0.0 s. Expect req & rem to be the same when function returns */
	zassert_equal(nanosleep(&req, &rem), 0);
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
	zassert_equal(nanosleep(&req, &req), 0);
	zassert_equal(errno, 0);
	zassert_equal(req.tv_sec, 0, "actual: %d expected: %d", req.tv_sec, 0);
	zassert_equal(req.tv_nsec, 0, "actual: %d expected: %d", req.tv_nsec, 0);
}

static void common(const uint32_t s, uint32_t ns)
{
	int r;
	uint32_t now;
	uint32_t then;
	struct timespec rem = {0, 0};
	struct timespec req = {s, ns};

	errno = 0;
	then = k_cycle_get_32();
	r = nanosleep(&req, &rem);
	now = k_cycle_get_32();

	zassert_equal(r, 0, "actual: %d expected: %d", r, -1);
	zassert_equal(errno, 0, "actual: %d expected: %d", errno, 0);
	zassert_equal(req.tv_sec, s, "actual: %d expected: %d", req.tv_sec, s);
	zassert_equal(req.tv_nsec, ns, "actual: %d expected: %d", req.tv_nsec, ns);
	zassert_equal(rem.tv_sec, 0, "actual: %d expected: %d", rem.tv_sec, 0);
	zassert_equal(rem.tv_nsec, 0, "actual: %d expected: %d", rem.tv_nsec, 0);

	uint64_t actual_ns = k_cyc_to_ns_ceil64((now - then));
	uint64_t exp_ns = (uint64_t)s * NSEC_PER_SEC + ns;
	/* round up to the nearest microsecond for k_busy_wait() */
	exp_ns = DIV_ROUND_UP(exp_ns, NSEC_PER_USEC) * NSEC_PER_USEC;

	/* lower bounds check */
	zassert_true(actual_ns >= exp_ns,
		"actual: %llu expected: %llu", actual_ns, exp_ns);

	/* TODO: Upper bounds check when hr timers are available */
}

ZTEST(posix_apis, test_nanosleep_execution)
{
	/* sleep for 1ns */
	common(0, 1);

	/* sleep for 1us + 1ns */
	common(0, 1001);

	/* sleep for 500000000ns */
	common(0, 500000000);

	/* sleep for 1s */
	common(1, 0);

	/* sleep for 1s + 1ns */
	common(1, 1);

	/* sleep for 1s + 1us + 1ns */
	common(1, 1001);
}
