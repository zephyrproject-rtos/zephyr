/*
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>

#include "test_pwm_loopback.h"

void test_main(void)
{
	struct test_pwm in;
	struct test_pwm out;

	get_test_pwms(&in, &out);

	k_object_access_grant(out.dev, k_current_get());
	k_object_access_grant(in.dev, k_current_get());

	ztest_test_suite(pwm_loopback_test,
			 ztest_user_unit_test(test_pulse_capture),
			 ztest_user_unit_test(test_pulse_capture_inverted),
			 ztest_user_unit_test(test_period_capture),
			 ztest_user_unit_test(test_period_capture_inverted),
			 ztest_user_unit_test(test_pulse_and_period_capture),
			 ztest_user_unit_test(test_capture_timeout),
			 ztest_unit_test(test_continuous_capture),
			 ztest_unit_test(test_capture_busy));
	ztest_run_test_suite(pwm_loopback_test);
}
