/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/ztest.h>

extern const struct pstate *soc_pstates[];
extern const size_t soc_pstates_count;

ZTEST(cpu_freq_energy_aware, test_pstate_timing_model)
{
	zassert_true(soc_pstates_count > 0U, "No P-states defined in devicetree");

	for (size_t i = 0U; i < soc_pstates_count; i++) {
		zassert_not_null(soc_pstates[i], "P-state %zu is NULL", i);
		zassert_true(soc_pstates[i]->frequency_hz > 0U,
			     "P-state %zu has no frequency", i);
	}
}

ZTEST(cpu_freq_energy_aware, test_select_pstate)
{
	const struct pstate *test_pstate;
	int ret;

	zassert_equal(cpu_freq_policy_select_pstate(NULL), -EINVAL,
		      "Expected -EINVAL for NULL pstate_out");

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	k_sched_lock();
#endif

	ret = cpu_freq_policy_select_pstate(&test_pstate);

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	k_sched_unlock();
#endif

	zassert_equal(ret, 0, "Expected success from cpu_freq_policy_select_pstate");
	zassert_not_null(test_pstate, "Expected selected P-state");
}

ZTEST_SUITE(cpu_freq_energy_aware, NULL, NULL, NULL, NULL, NULL);
