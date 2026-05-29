/*
 * Copyright (c) 2023 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST(timepoints, test_timepoint_api)
{
	k_timepoint_t timepoint;
	k_timeout_t timeout, remaining;

	timeout = K_NO_WAIT;
	timepoint = sys_timepoint_calc(timeout);
	zassert_true(sys_timepoint_expired(timepoint));
	remaining = sys_timepoint_timeout(timepoint);
	zassert_true(K_TIMEOUT_EQ(remaining, K_NO_WAIT));

	timeout = K_FOREVER;
	timepoint = sys_timepoint_calc(timeout);
	zassert_false(sys_timepoint_expired(timepoint));
	remaining = sys_timepoint_timeout(timepoint);
	zassert_true(K_TIMEOUT_EQ(remaining, K_FOREVER));

	timeout = K_SECONDS(1);
	timepoint = sys_timepoint_calc(timeout);
	zassert_false(sys_timepoint_expired(timepoint));
	remaining = sys_timepoint_timeout(timepoint);
	zassert_true(remaining.ticks <= timeout.ticks && remaining.ticks != 0);
	k_sleep(K_MSEC(1100));
	zassert_true(sys_timepoint_expired(timepoint));
	remaining = sys_timepoint_timeout(timepoint);
	zassert_true(K_TIMEOUT_EQ(remaining, K_NO_WAIT));
}

ZTEST(timepoints, test_comparison)
{
	k_timepoint_t a, b;

	a = sys_timepoint_calc(K_NO_WAIT);
	b = a;
	zassert_true(sys_timepoint_cmp(a, b) == 0);
	zassert_true(sys_timepoint_cmp(b, a) == 0);

	a = sys_timepoint_calc(K_FOREVER);
	b = a;
	zassert_true(sys_timepoint_cmp(a, b) == 0);
	zassert_true(sys_timepoint_cmp(b, a) == 0);

	a = sys_timepoint_calc(K_NO_WAIT);
	b = sys_timepoint_calc(K_MSEC(1));
	zassert_true(sys_timepoint_cmp(a, b) < 0);
	zassert_true(sys_timepoint_cmp(b, a) > 0);

	a = sys_timepoint_calc(K_MSEC(1));
	b = sys_timepoint_calc(K_FOREVER);
	zassert_true(sys_timepoint_cmp(a, b) < 0);
	zassert_true(sys_timepoint_cmp(b, a) > 0);

	a = sys_timepoint_calc(K_MSEC(1));
	b = a;
	zassert_true(sys_timepoint_cmp(a, b) == 0);
	zassert_true(sys_timepoint_cmp(b, a) == 0);

	a = sys_timepoint_calc(K_MSEC(100));
	b = sys_timepoint_calc(K_MSEC(200));
	zassert_true(sys_timepoint_cmp(a, b) < 0);
	zassert_true(sys_timepoint_cmp(b, a) > 0);

	a = sys_timepoint_calc(K_NO_WAIT);
	b = sys_timepoint_calc(K_FOREVER);
	zassert_true(sys_timepoint_cmp(a, b) < 0);
	zassert_true(sys_timepoint_cmp(b, a) > 0);
}

ZTEST_SUITE(timepoints, NULL, NULL, NULL, NULL, NULL);
