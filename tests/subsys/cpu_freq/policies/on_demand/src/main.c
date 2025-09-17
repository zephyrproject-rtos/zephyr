/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/cpu_freq.h>

LOG_MODULE_REGISTER(cpu_freq_on_demand_test, LOG_LEVEL_INF);

#define WAIT_US 1000

/*
 * Test APIs of on_demand CPU frequency policy.
 */
ZTEST(cpu_freq_on_demand, test_pstates)
{
	int ret;
	const struct pstate *test_pstate;

	/* Test invalid arg */
	zassert_equal(cpu_freq_policy_select_pstate(NULL), -EINVAL,
		      "Expected -EINVAL for NULL pstate_out");

	/* Simulate high-load and get pstate */
	k_busy_wait(WAIT_US);

	/* Get pstate after a moment of high-load */
	ret = cpu_freq_policy_select_pstate(&test_pstate);
	zassert_equal(ret, 0, "Expected success from cpu_freq_policy_select_pstate");

	int prev_threshold = test_pstate->load_threshold;

	/* Simulate low-load by sleeping, then getting pstate*/
	k_sleep(K_USEC(WAIT_US));

	/* Get pstate after a moment of low-load between calls to policy */
	ret = cpu_freq_policy_select_pstate(&test_pstate);
	zassert_equal(ret, 0, "Expected success from cpu_freq_policy_select_pstate");

	zassert_not_equal(test_pstate->load_threshold, prev_threshold,
			  "Expected different p-state after sleep");
}

ZTEST_SUITE(cpu_freq_on_demand, NULL, NULL, NULL, NULL, NULL);
