/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/drivers/rc/rc_loopback.h>
#include <zephyr/rc/rc.h>
#include <zephyr/ztest.h>

BUILD_ASSERT(RC_EVENT_FLAG_TOGGLE == 1U);
BUILD_ASSERT(RC_EVENT_FLAG_REPEAT == 2U);
BUILD_ASSERT((RC_RAW_DURATION_MASK & (RC_RAW_PULSE | RC_RAW_TIMEOUT | RC_RAW_OVERFLOW)) == 0U);
BUILD_ASSERT(RC_PROTO_XBOX_DVD < RC_PROTO_ZEPHYR_BASE);

static const struct device *const rc_dev = DEVICE_DT_GET(DT_NODELABEL(rc_loopback0));

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

ZTEST(rc_core, test_device_ready)
{
	zassert_true(device_is_ready(rc_dev));
}

ZTEST(rc_core, test_get_caps)
{
	struct rc_caps caps;

	zassert_ok(rc_get_caps(rc_dev, &caps));
	zassert_equal(caps.roles, RC_ROLE_RAW_RX);
	zassert_equal(caps.rx_resolution_ns, 1000U);
}

ZTEST(rc_core, test_rx_enable)
{
	zassert_ok(rc_rx_enable(rc_dev, true));
	zassert_true(rc_loopback_rx_is_enabled(rc_dev));
	zassert_ok(rc_rx_enable(rc_dev, false));
	zassert_false(rc_loopback_rx_is_enabled(rc_dev));
}

ZTEST_SUITE(rc_core, NULL, NULL, NULL, NULL, NULL);
