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
 * @brief Test the UTIL_INC boundary value
 *
 * @see UTIL_INC()
 */
ZTEST(sys_util, test_UTIL_INC)
{
	zassert_equal(1, UTIL_INC(0));
	zassert_equal(2, UTIL_INC(1));
	zassert_equal(4096, UTIL_INC(4095));
	zassert_equal(4097, UTIL_INC(4096));
}

/**
 * @brief Test the UTIL_DEC boundary value
 *
 * @see UTIL_DEC()
 */
ZTEST(sys_util, test_UTIL_DEC)
{
	zassert_equal(0, UTIL_DEC(0));
	zassert_equal(0, UTIL_DEC(1));
	zassert_equal(1, UTIL_DEC(2));
	zassert_equal(2, UTIL_DEC(3));
	zassert_equal(4094, UTIL_DEC(4095));
	zassert_equal(4095, UTIL_DEC(4096));
}

/**
 * @brief Test the UTIL_ADD boundary value
 *
 * @see UTIL_ADD()
 */
ZTEST(sys_util, test_UTIL_ADD)
{
	zassert_equal(0, UTIL_ADD(0, 0));
	zassert_equal(1, UTIL_ADD(1, 0));
	zassert_equal(1, UTIL_ADD(0, 1));
	zassert_equal(2, UTIL_ADD(1, 1));
	zassert_equal(3, UTIL_ADD(2, 1));
	zassert_equal(3, UTIL_ADD(1, 2));
	zassert_equal(4096, UTIL_ADD(4095, 1));
	zassert_equal(4097, UTIL_ADD(4095, 2));
	zassert_equal(4096, UTIL_ADD(1, 4095));
	zassert_equal(4097, UTIL_ADD(1, 4096));
}

/**
 * @brief Test the UTIL_SUB boundary value
 *
 * @see UTIL_SUB()
 */
ZTEST(sys_util, test_UTIL_SUB)
{
	zassert_equal(0, UTIL_SUB(0, 0));
	zassert_equal(1, UTIL_SUB(1, 0));
	zassert_equal(0, UTIL_SUB(0, 1));
	zassert_equal(0, UTIL_SUB(1, 1));
	zassert_equal(1, UTIL_SUB(2, 1));
	zassert_equal(0, UTIL_SUB(1, 2));
	zassert_equal(4095, UTIL_SUB(4096, 1));
	zassert_equal(4094, UTIL_SUB(4096, 2));
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
