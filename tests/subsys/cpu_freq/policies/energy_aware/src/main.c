/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/cpu_workload/cpu_workload.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TEST_WORKLOAD_CYCLES 10000ULL

extern const struct pstate *soc_pstates[];
extern const size_t soc_pstates_count;

static int workload_estimate_ret;
static uint64_t workload_cycles = TEST_WORKLOAD_CYCLES;
static const struct pstate *current_pstate;

int __wrap_cpu_workload_estimate_get(int cpu_id, struct cpu_workload_estimate *estimate)
{
	ARG_UNUSED(cpu_id);

	if (workload_estimate_ret != 0) {
		return workload_estimate_ret;
	}

	if (estimate == NULL) {
		return -EINVAL;
	}

	*estimate = (struct cpu_workload_estimate){
		.estimated_cycles = workload_cycles,
		.ready_backlog_cycles = workload_cycles,
		.source_mask = CPU_WORKLOAD_SOURCE_READY_BACKLOG,
		.confidence = 100U,
	};

	return 0;
}

const struct pstate *__wrap_cpu_freq_pstate_current_get(void)
{
	return current_pstate;
}

static const struct pstate *pstate_by_frequency(uint32_t frequency_hz)
{
	for (size_t i = 0U; i < soc_pstates_count; i++) {
		if (soc_pstates[i]->frequency_hz == frequency_hz) {
			return soc_pstates[i];
		}
	}

	return NULL;
}

static bool has_stub_pstates(void)
{
	return (pstate_by_frequency(100000000U) != NULL) &&
		(pstate_by_frequency(50000000U) != NULL) &&
		(pstate_by_frequency(10000000U) != NULL);
}

static void reset_policy_inputs(void *fixture)
{
	ARG_UNUSED(fixture);

	workload_estimate_ret = 0;
	workload_cycles = TEST_WORKLOAD_CYCLES;
	current_pstate = (soc_pstates_count > 0U) ? soc_pstates[0] : NULL;
}

static int select_pstate_locked(const struct pstate **pstate_out)
{
	unsigned int key;
	int ret;

	key = irq_lock();
	ret = cpu_freq_policy_select_pstate(pstate_out);
	irq_unlock(key);

	return ret;
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

ZTEST(cpu_freq_energy_aware, test_select_pstate_rejects_null_output)
{
	zassert_equal(select_pstate_locked(NULL), -EINVAL,
		      "Expected -EINVAL for NULL pstate_out");
}

ZTEST(cpu_freq_energy_aware, test_select_pstate_propagates_workload_error)
{
	const struct pstate *test_pstate = NULL;

	workload_estimate_ret = -EAGAIN;

	zassert_equal(select_pstate_locked(&test_pstate), -EAGAIN,
		      "Expected workload estimation error to be propagated");
	zassert_is_null(test_pstate, "Policy should not write a P-state on error");
}

ZTEST(cpu_freq_energy_aware, test_select_pstate)
{
	const struct pstate *test_pstate;
	int ret;

	ret = select_pstate_locked(&test_pstate);

	zassert_equal(ret, 0, "Expected success from cpu_freq_policy_select_pstate");
	zassert_not_null(test_pstate, "Expected selected P-state");
}

ZTEST(cpu_freq_energy_aware, test_select_pstate_for_configured_deadline)
{
	const struct pstate *expected_pstate;
	const struct pstate *selected_pstate = NULL;
	uint32_t expected_frequency_hz;
	int ret;

	if (!has_stub_pstates()) {
		ztest_test_skip();
		return;
	}

	switch (CONFIG_CPU_FREQ_POLICY_ENERGY_AWARE_DEADLINE_US) {
	case 2000:
		expected_frequency_hz = 10000000U;
		break;
	case 500:
		expected_frequency_hz = 50000000U;
		break;
	case 50:
		expected_frequency_hz = 100000000U;
		break;
	default:
		ztest_test_skip();
		return;
	}

	expected_pstate = pstate_by_frequency(expected_frequency_hz);
	zassert_not_null(expected_pstate, "Expected test P-state is missing");

	ret = select_pstate_locked(&selected_pstate);

	zassert_equal(ret, 0, "Expected policy selection to succeed");
	zassert_not_null(selected_pstate, "Expected selected P-state");
	zassert_equal(selected_pstate, expected_pstate,
		      "Expected %u Hz for deadline %u us, got %u Hz", expected_frequency_hz,
		      CONFIG_CPU_FREQ_POLICY_ENERGY_AWARE_DEADLINE_US,
		      selected_pstate->frequency_hz);
}

ZTEST(cpu_freq_energy_aware, test_policy_pstate_set)
{
	const struct pstate *test_pstate;

	zassert_is_null(cpu_freq_policy_pstate_set(NULL),
			"Expected NULL when applying NULL P-state");

	test_pstate = (soc_pstates_count > 0U) ? soc_pstates[0] : NULL;
	zassert_not_null(test_pstate, "No P-state defined in devicetree");
	zassert_equal(cpu_freq_policy_pstate_set(test_pstate), test_pstate,
		      "Expected policy setter to return applied P-state");
}

ZTEST_SUITE(cpu_freq_energy_aware, NULL, NULL, reset_policy_inputs, NULL, NULL);
