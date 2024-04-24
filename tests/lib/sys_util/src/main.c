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
 * @}
 */


/**
 * @defgroup sys_util_tests Sys Util Tests
 * @ingroup all_tests
 * @{
 * @}
 */

ZTEST_SUITE(sys_util, NULL, NULL, NULL, NULL, NULL);
