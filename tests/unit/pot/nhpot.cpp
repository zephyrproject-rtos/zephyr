/*
 * Copyright 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

static constexpr uint32_t val = NHPOT(42);
static constexpr uint64_t val64 = NHPOT(42 + BIT64(32));

template <typename T> static inline constexpr uint64_t nhpot(T x)
{
	return NHPOT(x);
}

ZTEST(pot, test_constexpr_NHPOT)
{
	zassert_equal(1, NHPOT(LLONG_MIN));
	zassert_equal(1, NHPOT(LONG_MIN));
	zassert_equal(1, NHPOT(INT_MIN));
	zassert_equal(1, NHPOT(-1));
	zassert_equal(1, NHPOT(0));
	zassert_equal(1, NHPOT(1));
	zassert_equal(2, NHPOT(2));
	zassert_equal(4, NHPOT(3));
	zassert_equal(4, NHPOT(4));
	zassert_equal(8, NHPOT(5));
	zassert_equal(BIT(31), NHPOT(BIT(31)));
	zassert_equal(BIT64(32), NHPOT(BIT(31) + 1));
	zassert_equal(BIT64(32), NHPOT(UINT32_MAX));
	zassert_equal(BIT64(32), NHPOT(BIT64(32)));
	zassert_equal(0, (uint32_t)NHPOT(BIT64(32)));
	zassert_equal(BIT64(33), NHPOT(BIT64(32) + 1));
	zassert_equal(BIT64(63), NHPOT(BIT64(63) - 1));
	zassert_equal(BIT64(63), NHPOT(BIT64(63)));
	zassert_equal(0, NHPOT(BIT64(63) + 1));
	zassert_equal(0, NHPOT(UINT64_MAX));

	zassert_equal(64, val);
	zassert_equal(64, nhpot(42));
	zassert_equal(BIT64(33), val64);
	zassert_equal(BIT64(33), nhpot(42 + BIT64(32)));
}
