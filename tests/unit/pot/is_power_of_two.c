/*
 * Copyright 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

ZTEST(pot, test_IS_POWER_OF_TWO)
{
	zassert_false(IS_POWER_OF_TWO(-1));
	zassert_false(IS_POWER_OF_TWO(0));
	zassert_true(IS_POWER_OF_TWO(1));
	zassert_true(IS_POWER_OF_TWO(2));
	zassert_false(IS_POWER_OF_TWO(3));
	zassert_true(IS_POWER_OF_TWO(4));
	zassert_true(IS_POWER_OF_TWO(BIT(30)));
	zassert_false(IS_POWER_OF_TWO(BIT(30) + 1));
	zassert_true(IS_POWER_OF_TWO(BIT64(32)));
	zassert_false(IS_POWER_OF_TWO(BIT64(32) + 1));
	zassert_true(IS_POWER_OF_TWO(BIT64(63)));
	zassert_false(IS_POWER_OF_TWO(BIT64(63) + 1));
	zassert_false(IS_POWER_OF_TWO(UINT64_MAX));
}
