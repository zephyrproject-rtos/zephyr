/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/rc.h>
#include <zephyr/rc/rc.h>
#include <zephyr/ztest.h>

BUILD_ASSERT(RC_EVENT_FLAG_TOGGLE == 1U);
BUILD_ASSERT(RC_EVENT_FLAG_REPEAT == 2U);
BUILD_ASSERT((RC_RAW_DURATION_MASK & (RC_RAW_PULSE | RC_RAW_TIMEOUT | RC_RAW_OVERFLOW)) == 0U);
BUILD_ASSERT(RC_PROTO_XBOX_DVD < RC_PROTO_ZEPHYR_BASE);

ZTEST(rc_core, test_raw_pulse)
{
	uint32_t word = RC_RAW_PULSE_US(560);

	zassert_true(rc_raw_is_pulse(word));
	zassert_equal(rc_raw_duration_us(word), 560U);
	zassert_equal(word & (RC_RAW_TIMEOUT | RC_RAW_OVERFLOW), 0U);
}

ZTEST(rc_core, test_raw_space)
{
	uint32_t word = RC_RAW_SPACE_US(1690);

	zassert_false(rc_raw_is_pulse(word));
	zassert_equal(rc_raw_duration_us(word), 1690U);
}

ZTEST(rc_core, test_raw_clamp)
{
	zassert_equal(rc_raw_duration_us(RC_RAW_PULSE_US(RC_RAW_DURATION_MASK + 1U)),
		      RC_RAW_DURATION_MASK);
	zassert_equal(rc_raw_duration_us(RC_RAW_SPACE_US(UINT32_MAX)), RC_RAW_DURATION_MASK);
	zassert_false(rc_raw_is_pulse(RC_RAW_SPACE_US(UINT32_MAX)));
}

ZTEST(rc_core, test_raw_zero)
{
	zassert_equal(RC_RAW_SPACE_US(0), 0U);
	zassert_equal(RC_RAW_PULSE_US(0), RC_RAW_PULSE);
}

ZTEST_SUITE(rc_core, NULL, NULL, NULL, NULL, NULL);
