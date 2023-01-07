/*
 * Copyright 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/sys/nhpot.h>
#include <zephyr/ztest.h>

static const uint32_t val = NHPOT(42);

ZTEST(nhpot, test_NHPOT)
{
	zassert_equal(1, NHPOT(0));
	zassert_equal(1, NHPOT(1));
	zassert_equal(2, NHPOT(2));
	zassert_equal(4, NHPOT(3));
	zassert_equal(4, NHPOT(4));
	zassert_equal(8, NHPOT(5));
	zassert_equal(2147483648UL, NHPOT(2147483648UL));
	zassert_equal(0, NHPOT(2147483649UL));
	zassert_equal(0, NHPOT(4294967295UL));

	zassert_equal(64, val);
}

static const uint64_t val64 = NHPOT64(42 + BIT64(32));

ZTEST(nhpot, test_NHPOT64)
{
	zassert_equal(1, NHPOT64(0));
	zassert_equal(1, NHPOT64(1));
	zassert_equal(2, NHPOT64(2));
	zassert_equal(4, NHPOT64(3));
	zassert_equal(4, NHPOT64(4));
	zassert_equal(8, NHPOT64(5));
	zassert_equal(2147483648UL, NHPOT64(2147483648UL));
	zassert_equal(4294967296ULL, NHPOT64(2147483649UL));
	zassert_equal(4294967296ULL, NHPOT64(4294967295UL));
	zassert_equal(9223372036854775808ULL, NHPOT64(9223372036854775807ULL));
	zassert_equal(9223372036854775808ULL, NHPOT64(9223372036854775808ULL));
	zassert_equal(0, NHPOT64(9223372036854775809ULL));
	zassert_equal(0, NHPOT64(18446744073709551615ULL));

	zassert_equal(BIT64(33), val64);
}

ZTEST(nhpot, test_nhpot)
{
	zassert_equal(1, nhpot(0));
	zassert_equal(1, nhpot(1));
	zassert_equal(2, nhpot(2));
	zassert_equal(4, nhpot(3));
	zassert_equal(4, nhpot(4));
	zassert_equal(8, nhpot(5));
	zassert_equal(2147483648UL, nhpot(2147483648UL));
	zassert_equal(0, nhpot((UINT32_MAX >> 1) + 2));
	zassert_equal(0, nhpot(UINT32_MAX));
}

ZTEST(nhpot, test_nhpot64)
{
	zassert_equal(1, nhpot64(0));
	zassert_equal(1, nhpot64(1));
	zassert_equal(2, nhpot64(2));
	zassert_equal(4, nhpot64(3));
	zassert_equal(4, nhpot64(4));
	zassert_equal(8, nhpot64(5));
	zassert_equal(2147483648UL, nhpot64(2147483648UL));
	zassert_equal(4294967296ULL, nhpot64(2147483649UL));
	zassert_equal(4294967296ULL, nhpot64(4294967295UL));
	zassert_equal(9223372036854775808ULL, nhpot64(9223372036854775807ULL));
	zassert_equal(9223372036854775808ULL, nhpot64(9223372036854775808ULL));
	zassert_equal(0, nhpot64((UINT64_MAX >> 1) + 2));
	zassert_equal(0, nhpot64(UINT64_MAX));
}
