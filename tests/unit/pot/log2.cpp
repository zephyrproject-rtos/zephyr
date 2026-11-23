/*
 * Copyright 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

static constexpr uint8_t val = LOG2(42);
static constexpr uint8_t val64 = LOG2(42 + BIT64(32));

template <typename T> static inline constexpr uint8_t log2(T x)
{
	return LOG2(x);
}

ZTEST(pot, test_constexpr_LOG2)
{
	zassert_equal(-1, LOG2(0));
	zassert_equal(0, LOG2(1));
	zassert_equal(1, LOG2(2));
	zassert_equal(1, LOG2(3));
	zassert_equal(2, LOG2(4));
	zassert_equal(2, LOG2(5));
	zassert_equal(31, LOG2(BIT(31)));
	zassert_equal(31, LOG2(BIT(31) + 1));
	zassert_equal(31, LOG2(UINT32_MAX));
	zassert_equal(32, LOG2(BIT64(32)));
	zassert_equal(62, LOG2(BIT64(63) - 1));
	zassert_equal(63, LOG2(BIT64(63)));
	zassert_equal(63, LOG2(BIT64(63) + 1));
	zassert_equal(63, LOG2(UINT64_MAX));

	zassert_equal(5, val);
	zassert_equal(5, log2(42));
	zassert_equal(32, val64);
	zassert_equal(32, log2(42 + BIT64(32)));
}
