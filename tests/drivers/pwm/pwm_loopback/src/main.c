/*
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "test_pwm_loopback.h"

static void *pwm_loopback_setup(void)
{
	struct test_pwm in;
	struct test_pwm out;

	get_test_pwms(&out, &in);

	k_object_access_grant(out.dev, k_current_get());
	k_object_access_grant(in.dev, k_current_get());

	return NULL;
}

static void pwm_loopback_after(void *f)
{
	struct test_pwm in;
	struct test_pwm out;
	int err;

	ARG_UNUSED(f);

	get_test_pwms(&out, &in);

	err = pwm_disable_capture(in.dev, in.pwm);
	zassert_equal(err, 0, "failed to disable pwm capture (err %d)", err);
}

ZTEST_SUITE(pwm_loopback, NULL, pwm_loopback_setup, NULL, pwm_loopback_after, NULL);
