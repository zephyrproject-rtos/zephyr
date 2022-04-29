/*
 * Copyright (c) 2022 Intel Corporation+
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/async.h>
#include <zephyr/kernel.h>
#include <ztest.h>


static struct k_poll_signal sig;

void test_async_signal(void)
{
	unsigned int signaled;
	int result;

	k_poll_signal_init(&sig);
	async_signal_cb(2, &sig);
	k_poll_signal_check(&sig, &signaled, &result);

	zassert_equal(signaled, 1, "expected signal");
	zassert_equal(result, 2, "expected result");
}

void test_main(void)
{
	ztest_test_suite(test_async,
			 ztest_unit_test(test_async_signal)
			 );
	ztest_run_test_suite(test_async);
}
