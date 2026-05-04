/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/power/qos.h>
#include <zephyr/ztest.h>

extern const struct pstate *soc_pstates[];
extern const size_t soc_pstates_count;

static const struct pstate *lowest_pstate(void)
{
	const struct pstate *lowest = NULL;

	for (size_t i = 0U; i < soc_pstates_count; i++) {
		if ((lowest == NULL) || (soc_pstates[i]->frequency_hz < lowest->frequency_hz)) {
			lowest = soc_pstates[i];
		}
	}

	return lowest;
}

static const struct pstate *highest_pstate(void)
{
	const struct pstate *highest = NULL;

	for (size_t i = 0U; i < soc_pstates_count; i++) {
		if ((highest == NULL) || (soc_pstates[i]->frequency_hz > highest->frequency_hz)) {
			highest = soc_pstates[i];
		}
	}

	return highest;
}

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

ZTEST(cpu_freq_energy_aware, test_pstate_max_constraint)
{
	struct power_qos_pstate_request max_req;
	const struct pstate *test_pstate;
	const struct pstate *lowest = lowest_pstate();
	int ret;

	zassert_not_null(lowest);

	power_qos_max_pstate_request_add(&max_req, lowest);
	ret = cpu_freq_policy_select_pstate(&test_pstate);
	power_qos_max_pstate_request_remove(&max_req);

	zassert_equal(ret, 0, "Expected success from constrained selection");
	zassert_equal(test_pstate, lowest, "Expected max constraint to force lowest P-state");
}

ZTEST(cpu_freq_energy_aware, test_pstate_constraint_conflict)
{
	struct power_qos_pstate_request min_req;
	struct power_qos_pstate_request max_req;
	const struct pstate *test_pstate;
	const struct pstate *lowest = lowest_pstate();
	const struct pstate *highest = highest_pstate();
	int ret;

	zassert_not_null(lowest);
	zassert_not_null(highest);
	zassert_not_equal(lowest, highest, "Need at least two P-states for conflict test");

	power_qos_min_pstate_request_add(&min_req, highest);
	power_qos_max_pstate_request_add(&max_req, lowest);
	ret = cpu_freq_policy_select_pstate(&test_pstate);
	power_qos_min_pstate_request_remove(&min_req);
	power_qos_max_pstate_request_remove(&max_req);

	zassert_equal(ret, -ERANGE, "Expected conflict to be reported");
}

ZTEST_SUITE(cpu_freq_energy_aware, NULL, NULL, NULL, NULL, NULL);
