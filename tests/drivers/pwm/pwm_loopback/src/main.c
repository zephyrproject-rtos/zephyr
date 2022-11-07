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

ZTEST_SUITE(pwm_loopback, NULL, pwm_loopback_setup, NULL, NULL, NULL);
