/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>

const struct device *get_pwm_device(void);

static void *pwm_basic_setup(void)
{
	const struct device *dev = get_pwm_device();

	zassert_true(device_is_ready(dev), "PWM device is not ready");
	k_object_access_grant(dev, k_current_get());

	return NULL;
}

ZTEST_SUITE(pwm_basic, NULL, pwm_basic_setup, NULL, NULL, NULL);
