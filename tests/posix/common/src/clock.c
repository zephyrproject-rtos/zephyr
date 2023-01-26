/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/posix/time.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/unistd.h>

#define SLEEP_SECONDS 1
#define CLOCK_INVALID -1

ZTEST(posix_apis, test_posix_clock)
{
	int64_t nsecs_elapsed, secs_elapsed;
	struct timespec ts, te;

	printk("POSIX clock APIs\n");

	/* TESTPOINT: Pass invalid clock type */
	zassert_equal(clock_gettime(CLOCK_INVALID, &ts), -1,
			NULL);
	zassert_equal(errno, EINVAL);

	zassert_ok(clock_gettime(CLOCK_MONOTONIC, &ts));
	zassert_ok(k_sleep(K_SECONDS(SLEEP_SECONDS)));
	zassert_ok(clock_gettime(CLOCK_MONOTONIC, &te));

	if (te.tv_nsec >= ts.tv_nsec) {
		secs_elapsed = te.tv_sec - ts.tv_sec;
		nsecs_elapsed = te.tv_nsec - ts.tv_nsec;
	} else {
		nsecs_elapsed = NSEC_PER_SEC + te.tv_nsec - ts.tv_nsec;
		secs_elapsed = (te.tv_sec - ts.tv_sec - 1);
	}

	/*TESTPOINT: Check if POSIX clock API test passes*/
	zassert_equal(secs_elapsed, SLEEP_SECONDS,
			"POSIX clock API test failed");

	printk("POSIX clock APIs test done\n");
}

ZTEST(posix_apis, test_posix_realtime)
{
	int ret;
	struct timespec rts, mts;
	struct timeval tv;

	ret = clock_gettime(CLOCK_MONOTONIC, &mts);
	zassert_equal(ret, 0, "Fail to get monotonic clock");

	ret = clock_gettime(CLOCK_REALTIME, &rts);
	zassert_equal(ret, 0, "Fail to get realtime clock");

	/* Set a particular time.  In this case, the output of:
	 * `date +%s -d 2018-01-01T15:45:01Z`
	 */
	struct timespec nts;
	nts.tv_sec = 1514821501;
	nts.tv_nsec = NSEC_PER_SEC / 2U;

	/* TESTPOINT: Pass invalid clock type */
	zassert_equal(clock_settime(CLOCK_INVALID, &nts), -1,
			NULL);
	zassert_equal(errno, EINVAL);

	ret = clock_settime(CLOCK_MONOTONIC, &nts);
	zassert_not_equal(ret, 0, "Should not be able to set monotonic time");

	ret = clock_settime(CLOCK_REALTIME, &nts);
	zassert_equal(ret, 0, "Fail to set realtime clock");

	/*
	 * Loop 20 times, sleeping a little bit for each, making sure
	 * that the arithmetic roughly makes sense.  This tries to
	 * catch all of the boundary conditions of the clock to make
	 * sure there are no errors in the arithmetic.
	 */
	int64_t last_delta = 0;
	for (int i = 1; i <= 20; i++) {
		usleep(USEC_PER_MSEC * 90U);
		ret = clock_gettime(CLOCK_REALTIME, &rts);
		zassert_equal(ret, 0, "Fail to read realtime clock");

		int64_t delta =
			((int64_t)rts.tv_sec * NSEC_PER_SEC -
			 (int64_t)nts.tv_sec * NSEC_PER_SEC) +
			((int64_t)rts.tv_nsec - (int64_t)nts.tv_nsec);

		/* Make the delta milliseconds. */
		delta /= (NSEC_PER_SEC / 1000U);

		zassert_true(delta > last_delta, "Clock moved backward");
		int64_t error = delta - last_delta;

		/* printk("Delta %d: %lld\n", i, delta); */

		/* Allow for a little drift upward, but not
		 * downward
		 */
		zassert_true(error >= 90, "Clock inaccurate %d", error);
		zassert_true(error <= 110, "Clock inaccurate %d", error);

		last_delta = delta;
	}

	/* Validate gettimeofday API */
	ret = gettimeofday(&tv, NULL);
	zassert_equal(ret, 0);

	ret = clock_gettime(CLOCK_REALTIME, &rts);
	zassert_equal(ret, 0);

	/* TESTPOINT: Check if time obtained from
	 * gettimeofday is same or more than obtained
	 * from clock_gettime
	 */
	zassert_true(rts.tv_sec >= tv.tv_sec, "gettimeofday didn't"
			" provide correct result");
	zassert_true(rts.tv_nsec >= tv.tv_usec * NSEC_PER_USEC,
			"gettimeofday didn't provide correct result");
}

static inline bool ts_gt(const struct timespec *a, const struct timespec *b)
{
	__ASSERT_NO_MSG(a->tv_nsec < NSEC_PER_SEC);
	__ASSERT_NO_MSG(a->tv_nsec >= 0);
	__ASSERT_NO_MSG(b->tv_nsec < NSEC_PER_SEC);
	__ASSERT_NO_MSG(b->tv_nsec >= 0);

	if (a->tv_sec > b->tv_sec) {
		return true;
	}

	if (a->tv_sec < b->tv_sec) {
		return false;
	}

	if (a->tv_nsec > b->tv_nsec) {
		return true;
	}

	return false;
}

static inline void ts_print(const char *label, const struct timespec *ts)
{
	printk("%s: {%" PRIu64 ", %" PRIu64 "}\n", label, (uint64_t)ts->tv_sec,
	       (uint64_t)ts->tv_nsec);
}

ZTEST(posix_apis, test_clock_gettime_rollover)
{
	uint64_t t;
	struct timespec ts[3];
	const uint64_t rollover_s = UINT64_MAX / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	printk("CONFIG_SYS_CLOCK_TICKS_PER_SEC: %u\n", CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	printk("rollover_s: %" PRIu64 "\n", rollover_s);

	t = UINT64_MAX - 1;
	/* align to tick boundary */
	k_sleep(K_TICKS(1));
	sys_clock_tick_set(t);
	zassert_ok(clock_gettime(CLOCK_MONOTONIC, &ts[0]));
	ts_print("t-1", &ts[0]);
	zassert_equal(rollover_s, ts[0].tv_sec);

	t = UINT64_MAX;
	/* align to tick boundary */
	k_sleep(K_TICKS(1));
	sys_clock_tick_set(t);
	zassert_ok(clock_gettime(CLOCK_MONOTONIC, &ts[1]));
	ts_print("t+0", &ts[1]);
	zassert_equal(rollover_s, ts[1].tv_sec);
	zassert_true(ts_gt(&ts[1], &ts[0]));

	t = UINT64_MAX + 1;
	/* align to tick boundary */
	k_sleep(K_TICKS(1));
	sys_clock_tick_set(t);
	zassert_ok(clock_gettime(CLOCK_MONOTONIC, &ts[2]));
	ts_print("t+1", &ts[2]);
	zassert_equal(0, ts[2].tv_sec);
	zassert_true(ts_gt(&ts[1], &ts[2]));
}
