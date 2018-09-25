/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#define SLEEP_MS 100

void test_sysclock_state(int state)
{
	/*sysclock is suspended as system devcie*/
	/*sysclock is resumed as system device*/
	return;
}

/*todo: leverage basic tests from sysclock test set*/
void test_sysclock_func(void)
{
	static s64_t t0;

	/**TESTPOINT: verify task_sleep duration*/
	t0 = k_uptime_get();
	k_sleep(SLEEP_MS);
	zassert_true(k_uptime_delta(&t0) >= SLEEP_MS, NULL);
}
