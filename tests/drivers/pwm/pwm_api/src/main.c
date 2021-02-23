/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <ztest.h>
#include <device.h>

const struct device *get_pwm_device(void);
void test_pwm_usec(void);
void test_pwm_cycle(void);
void test_pwm_nsec(void);

void test_main(void)
{
	const struct device *dev = get_pwm_device();

	zassert_true(device_is_ready(dev), "PWM device is not ready");
	k_object_access_grant(dev, k_current_get());

	ztest_test_suite(pwm_basic_test,
			 ztest_user_unit_test(test_pwm_usec),
			 ztest_user_unit_test(test_pwm_nsec),
			 ztest_user_unit_test(test_pwm_cycle));
	ztest_run_test_suite(pwm_basic_test);
}
