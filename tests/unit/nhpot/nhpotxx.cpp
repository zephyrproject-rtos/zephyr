/*
 * Copyright 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/nhpot.h>
#include <zephyr/ztest.h>

static constexpr uint32_t val = nhpot(42);

ZTEST(nhpot, test_constexpr_nhpot)
{
	zassert_equal(64, val);
}

static constexpr uint64_t val64 = nhpot64(42 + BIT64(32));

ZTEST(nhpot, test_constexpr_nhpot64)
{
	zassert_equal(BIT64(33), val64);
}
