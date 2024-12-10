/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include "test_pwm_to_gpio_loopback.h"

static void *pwm_loopback_setup(void)
{
	struct pwm_dt_spec out;
	struct gpio_dt_spec in;

	get_test_devices(&out, &in);

	k_object_access_grant(out.dev, k_current_get());
	k_object_access_grant(in.port, k_current_get());

	TC_PRINT("Testing PWM device %s, channel %d\n", out.dev->name, out.channel);
	TC_PRINT("GPIO loopback at %s, pin %d\n", in.port->name, in.pin);
	TC_PRINT("===================================================================\n");

	return NULL;
}

ZTEST_SUITE(pwm_loopback, NULL, pwm_loopback_setup, NULL, NULL, NULL);
