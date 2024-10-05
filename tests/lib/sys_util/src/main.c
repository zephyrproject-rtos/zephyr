/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

/**
 * @addtogroup sys_util_tests
 * @{
 */

ZTEST(sys_util, test_SCALE)
{
	/* Test for a few arbitrary values */
	zassert_equal(SCALE(3, 0, 10, 0, 100), 30);
	zassert_equal(SCALE(-3, -10, 0, -100, 0), -30);
	zassert_equal(SCALE(10, -100, 100, -10, 10), 1);
	zassert_equal(SCALE(0, -10, 40, -50, 0), -40);
	zassert_equal(SCALE(0, -128, 127, 0, 2), 1);
	zassert_equal(SCALE(5, -50, 5000, -1000, 10), -989);

	/* Test for i = 1..60 and o = 60..1 */
	for (int i = 1; i < 61; i++) {
		int o = 61 - i;
		int64_t imin = -(1ll << i);
		int64_t imax = (1ll << i);
		int64_t omin = -(1ll << o);
		int64_t omax = (1ll << o);

		/* Special case: the output range can be [0, 0] */

		zassert_equal(SCALE(imin, imin, imax, 0, 0), 0);
		zassert_equal(SCALE(0, imin, imax, 0, 0), 0);
		zassert_equal(SCALE(imax, imin, imax, 0, 0), 0);

		zassert_equal(SCALE(0, 0, imax, 0, 0), 0);
		zassert_equal(SCALE(imax, 0, imax, 0, 0), 0);

		zassert_equal(SCALE(imin, imin, 0, 0, 0), 0);
		zassert_equal(SCALE(0, imin, 0, 0, 0), 0);

		/* Test the extreme cases */

		zassert_equal(SCALE(imin, imin, imax, omin, omax), omin);
		zassert_equal(SCALE(0, imin, imax, omin, omax), 0);
		zassert_equal(SCALE(imax, imin, imax, omin, omax), omax);

		zassert_equal(SCALE(0, 0, imax, omin, omax), omin);
		zassert_equal(SCALE(imax, 0, imax, omin, omax), omax);

		zassert_equal(SCALE(imin, imin, 0, omin, omax), omin);
		zassert_equal(SCALE(0, imin, 0, omin, omax), omax);

		zassert_equal(SCALE(imin, imin, imax, 0, omax), 0);
		zassert_equal(SCALE(0, imin, imax, 0, omax), omax / 2);
		zassert_equal(SCALE(imax, imin, imax, 0, omax), omax);

		zassert_equal(SCALE(imin, imin, imax, omin, 0), omin);
		zassert_equal(SCALE(0, imin, imax, omin, 0), omin / 2);
		zassert_equal(SCALE(imax, imin, imax, omin, 0), 0);

		zassert_equal(SCALE(0, 0, imax, 0, omax), 0);
		zassert_equal(SCALE(imax, 0, imax, 0, omax), omax);

		zassert_equal(SCALE(0, 0, imax, omin, 0), omin);
		zassert_equal(SCALE(imax, 0, imax, omin, 0), 0);

		zassert_equal(SCALE(imin, imin, 0, 0, omax), 0);
		zassert_equal(SCALE(0, imin, 0, 0, omax), omax);

		zassert_equal(SCALE(imin, imin, 0, omin, 0), omin);
		zassert_equal(SCALE(0, imin, 0, omin, 0), 0);

		zassert_equal(SCALE(0, 0, imax, 0, omax), 0);
		zassert_equal(SCALE(imax, 0, imax, 0, omax), omax);
	}
}

/**
 * @brief Test wait_for works as expected with typical use cases
 *
 * @see WAIT_FOR()
 */

ZTEST(sys_util, test_wait_for)
{
	uint32_t start, end, expected;

	zassert_true(WAIT_FOR(true, 0, NULL), "true, no wait, NULL");
	zassert_true(WAIT_FOR(true, 0, k_yield()), "true, no wait, yield");
	zassert_false(WAIT_FOR(false, 0, k_yield()), "false, no wait, yield");
	zassert_true(WAIT_FOR(true, 1, k_yield()), "true, 1usec, yield");
	zassert_false(WAIT_FOR(false, 1, k_yield()), "false, 1usec, yield");
	zassert_true(WAIT_FOR(true, 1000, k_yield()), "true, 1msec, yield");


	expected = 1000*(sys_clock_hw_cycles_per_sec()/USEC_PER_SEC);
	start = k_cycle_get_32();
	zassert_false(WAIT_FOR(false, 1000, k_yield()), "true, 1msec, yield");
	end = k_cycle_get_32();
	zassert_true(end-start >= expected, "wait for 1ms");
}

/**
 * @brief Test NUM_VA_ARGS works as expected with typical use cases
 *
 * @see NUM_VA_ARGS()
 */

ZTEST(sys_util, test_NUM_VA_ARGS)
{
	zassert_equal(0, NUM_VA_ARGS());
	zassert_equal(1, NUM_VA_ARGS(_1));
	zassert_equal(2, NUM_VA_ARGS(_1, _2));
	/* support up to 63 args */
	zassert_equal(63, NUM_VA_ARGS(LISTIFY(63, ~, (,))));
}

/**
 * @brief Test NUM_VA_ARGS_LESS_1 works as expected with typical use cases
 *
 * @see NUM_VA_ARGS_LESS_1()
 */

ZTEST(sys_util, test_NUM_VA_ARGS_LESS_1)
{
	zassert_equal(0, NUM_VA_ARGS_LESS_1());
	zassert_equal(0, NUM_VA_ARGS_LESS_1(_1));
	zassert_equal(1, NUM_VA_ARGS_LESS_1(_1, _2));
	/* support up to 64 args */
	zassert_equal(63, NUM_VA_ARGS_LESS_1(LISTIFY(64, ~, (,))));
}
/**
 * @}
 */

/**
 * @defgroup sys_util_tests Sys Util Tests
 * @ingroup all_tests
 * @{
 * @}
 */

ZTEST_SUITE(sys_util, NULL, NULL, NULL, NULL, NULL);
