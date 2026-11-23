/*
 * Copyright 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

static const uint8_t val = LOG2CEIL(42);
static const uint8_t val64 = LOG2CEIL(42 + BIT64(32));

ZTEST(pot, test_LOG2CEIL)
{
	zassert_equal(0, LOG2CEIL(LLONG_MIN));
	zassert_equal(0, LOG2CEIL(LONG_MIN));
	zassert_equal(0, LOG2CEIL(INT_MIN));
	zassert_equal(0, LOG2CEIL(-1));
	zassert_equal(0, LOG2CEIL(0));
	zassert_equal(0, LOG2CEIL(1));
	zassert_equal(1, LOG2CEIL(2));
	zassert_equal(2, LOG2CEIL(3));
	zassert_equal(2, LOG2CEIL(4));
	zassert_equal(3, LOG2CEIL(5));
	zassert_equal(31, LOG2CEIL(BIT(31)));
	zassert_equal(32, LOG2CEIL(BIT(31) + 1));
	zassert_equal(32, LOG2CEIL(UINT32_MAX));
	zassert_equal(32, LOG2CEIL(BIT64(32)));
	zassert_equal(33, LOG2CEIL(BIT64(32) + 1));
	zassert_equal(63, LOG2CEIL(BIT64(63) - 1));
	zassert_equal(63, LOG2CEIL(BIT64(63)));
	zassert_equal(64, LOG2CEIL(BIT64(63) + 1));
	zassert_equal(64, LOG2CEIL(UINT64_MAX));

	zassert_equal(6, val);
	zassert_equal(33, val64);
}
