/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#ifdef CONFIG_TIMEOUT_64BIT
/**
 * Verify that absolute timeout sums are handled correctly
 */
ZTEST(timeout, test_timeout_sum_absolute)
{
	k_timeout_t abs_timeout = K_TIMEOUT_ABS_TICKS(1000);
	k_timeout_t result;

	/* Two absolute timeouts should result in K_FOREVER */

	result = K_TIMEOUT_SUM(abs_timeout, abs_timeout);
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	/* Absolute with K_FOREVER should result in K_FOREVER */

	result = K_TIMEOUT_SUM(abs_timeout, K_FOREVER);
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_FOREVER, abs_timeout);
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	/* Absolute with K_NO_WAIT should return the absolute */

	result = K_TIMEOUT_SUM(abs_timeout, K_NO_WAIT);
	zassert_true(K_TIMEOUT_EQ(result, abs_timeout),
		     "Expected K_TIMEOUT_ABS_TICKS(1000)");

	result = K_TIMEOUT_SUM(K_NO_WAIT, abs_timeout);
	zassert_true(K_TIMEOUT_EQ(result, abs_timeout),
		     "Expected K_TIMEOUT_ABS_TICKS(1000)");

	/* Absolute + relative (no underflow) should return a new absolute */

	result = K_TIMEOUT_SUM(abs_timeout, K_TICKS(100));
	zassert_true(K_TIMEOUT_EQ(result, K_TIMEOUT_ABS_TICKS(1100)),
		     "Expected K_TIMEOUT_ABS_TICKS(1100)");

	result = K_TIMEOUT_SUM(K_TICKS(100), abs_timeout);
	zassert_true(K_TIMEOUT_EQ(result, K_TIMEOUT_ABS_TICKS(1100)),
		     "Expected K_TIMEOUT_ABS_TICKS(1100)");

	/* Limit testing: small absolute + large relative -- absolute 1st */

	result = K_TIMEOUT_SUM(K_TIMEOUT_ABS_TICKS(5), K_TICKS(INT64_MAX - 4));
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_TIMEOUT_ABS_TICKS(5), K_TICKS(INT64_MAX - 5));
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_TIMEOUT_ABS_TICKS(5), K_TICKS(INT64_MAX - 6));
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(INT64_MIN)),
		     "Expected INT64_MIN ticks");

	result = K_TIMEOUT_SUM(K_TIMEOUT_ABS_TICKS(5), K_TICKS(INT64_MAX - 7));
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(INT64_MIN + 1)),
		     "Expected INT64_MIN + 1 ticks");

	/* Limit testing: small absolute + large relative -- relative 1st */

	result = K_TIMEOUT_SUM(K_TICKS(INT64_MAX - 4), K_TIMEOUT_ABS_TICKS(5));
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_TICKS(INT64_MAX - 5), K_TIMEOUT_ABS_TICKS(5));
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_TICKS(INT64_MAX - 6), K_TIMEOUT_ABS_TICKS(5));
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(INT64_MIN)),
		     "Expected INT64_MIN ticks");

	result = K_TIMEOUT_SUM(K_TICKS(INT64_MAX - 7), K_TIMEOUT_ABS_TICKS(5));
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(INT64_MIN + 1)),
		     "Expected INT64_MIN + 1 ticks");

	/* Limit testing large absolute + small relative -- absolute 1st */

	result = K_TIMEOUT_SUM(K_TIMEOUT_ABS_TICKS(INT64_MAX - 5), K_TICKS(6));
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_TIMEOUT_ABS_TICKS(INT64_MAX - 6), K_TICKS(6));
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_TIMEOUT_ABS_TICKS(INT64_MAX - 7), K_TICKS(6));
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(INT64_MIN)),
		     "Expected INT64_MIN ticks");

	result = K_TIMEOUT_SUM(K_TIMEOUT_ABS_TICKS(INT64_MAX - 8), K_TICKS(6));
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(INT64_MIN + 1)),
		     "Expected INT64_MIN + 1 ticks");

	/* Limit testing large absolute + small relative -- relative  1st */

	result = K_TIMEOUT_SUM(K_TICKS(6), K_TIMEOUT_ABS_TICKS(INT64_MAX - 5));
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_TICKS(6), K_TIMEOUT_ABS_TICKS(INT64_MAX - 6));
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_TICKS(6), K_TIMEOUT_ABS_TICKS(INT64_MAX - 7));
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(INT64_MIN)),
		     "Expected INT64_MIN ticks");

	result = K_TIMEOUT_SUM(K_TICKS(6), K_TIMEOUT_ABS_TICKS(INT64_MAX - 8));
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(INT64_MIN + 1)),
		     "Expected INT64_MIN + 1 ticks");
}
#endif

/**
 * Verify that relative timeout sums are handled correctly
 */
ZTEST(timeout, test_timeout_sum_relative)
{
	k_timeout_t result;

	/* Verify that normal sums work as expected */

	result = K_TIMEOUT_SUM(K_TICKS(1), K_TICKS(2));
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(3)), "Expected 3 ticks");

	/* K_NO_WAIT + X should return X */

	result = K_TIMEOUT_SUM(K_NO_WAIT, K_TICKS(1));
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(1)), "Expected 1 tick");

	result = K_TIMEOUT_SUM(K_TICKS(1), K_NO_WAIT);
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(1)), "Expected 1 tick");

	result = K_TIMEOUT_SUM(K_NO_WAIT, K_NO_WAIT);
	zassert_true(K_TIMEOUT_EQ(result, K_NO_WAIT), "Expected K_NO_WAIT");

	/* K_FOREVER + anything should return K_FOREVER */

	result = K_TIMEOUT_SUM(K_TICKS(1), K_FOREVER);
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_FOREVER, K_TICKS(1));
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_FOREVER, K_NO_WAIT);
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_NO_WAIT, K_FOREVER);
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_FOREVER, K_FOREVER);
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	/* Behavior at limits */

	result = K_TIMEOUT_SUM(K_TICKS(K_TICK_MAX - 1), K_TICKS(1));
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(K_TICK_MAX)), "Expected K_TICK_MAX ticks");

	result = K_TIMEOUT_SUM(K_TICKS(K_TICK_MAX - 1), K_TICKS(2));
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");

	result = K_TIMEOUT_SUM(K_TICKS(K_TICK_MAX), K_NO_WAIT);
	zassert_true(K_TIMEOUT_EQ(result, K_TICKS(K_TICK_MAX)), "Expected K_TICK_MAX ticks");

	result = K_TIMEOUT_SUM(K_TICKS(K_TICK_MAX), K_TICKS(1));
	zassert_true(K_TIMEOUT_EQ(result, K_FOREVER), "Expected K_FOREVER");
}

ZTEST_SUITE(timeout, NULL, NULL, NULL, NULL, NULL);
