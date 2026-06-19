/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Static SCMI cpu_freq tests.
 *
 * Validates the SCMI Performance Domain cpu_freq backend without relying
 * on the cpu_freq timer.  The timer interval is set to 1000000 ms so it
 * never fires during the test, giving the test thread exclusive control
 * over frequency transitions.
 *
 * Test cases:
 *
 *   test_pstates_order
 *     Verifies that /performance-states DTS children are listed in
 *     strictly decreasing load-threshold order, as required by the
 *     on_demand policy scanner.
 *
 *   test_pstate_set_get_roundtrip
 *     Calls cpu_freq_pstate_set() for every DTS pstate and verifies
 *     via scmi_perf_level_get() that the SCMI platform firmware actually switched
 *     the CPU frequency.  This is the hardware-level proof that the
 *     SCMI PERFORMANCE_LEVEL_SET command was executed correctly.
 *
 *   test_on_demand_policy_load_driven
 *     Verifies that the on_demand policy selects a higher-frequency
 *     pstate after a busy-wait period and a lower-frequency pstate
 *     after an idle period.  Only the policy selection is tested here;
 *     the actual frequency switch is covered by test_pstate_set_get_roundtrip.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/drivers/firmware/scmi/perf.h>

LOG_MODULE_REGISTER(scmi_cpu_freq_basic, LOG_LEVEL_INF);

/* 100 ms busy-wait to drive cpu_load_metric to ~100% */
#define BUSY_WAIT_US   100000U

/* 200 ms sleep to drive cpu_load_metric to ~0% */
#define SLEEP_MS       200U

/* Wait after cpu_freq_pstate_set() before reading back the currently level. */
#define SETTLE_MS      10U

extern const struct pstate *soc_pstates[];
extern const size_t soc_pstates_count;

struct scmi_perf_pstate_cfg {
	uint32_t domain_id;
	uint32_t perf_level_khz;
};

static uint32_t get_current_khz(uint32_t domain_id)
{
	uint32_t khz = 0;

	zassert_ok(scmi_perf_level_get(domain_id, &khz),
		   "scmi_perf_level_get domain=%u failed", domain_id);
	return khz;
}

ZTEST(scmi_cpu_freq_basic, test_pstates_order)
{
	zassert_true(soc_pstates_count > 1,
		     "Need at least 2 P-states to test ordering");

	for (size_t i = 1; i < soc_pstates_count; i++) {
		zassert_true(
			soc_pstates[i]->load_threshold <
			soc_pstates[i - 1]->load_threshold,
			"P-state[%zu] threshold (%u) must be < "
			"P-state[%zu] threshold (%u)",
			i, soc_pstates[i]->load_threshold,
			i - 1, soc_pstates[i - 1]->load_threshold);
	}

	LOG_INF("P-state order OK: %zu states in decreasing threshold order",
		soc_pstates_count);
}

ZTEST(scmi_cpu_freq_basic, test_pstate_set_get_roundtrip)
{
	/* Iterate lowest → highest to minimise voltage stress */
	for (int i = (int)soc_pstates_count - 1; i >= 0; i--) {
		const struct pstate *state = soc_pstates[i];
		const struct scmi_perf_pstate_cfg *cfg =
			(const struct scmi_perf_pstate_cfg *)state->config;
		uint32_t actual_khz;

		LOG_INF("Setting P-state[%d]: domain=%u %u kHz",
			i, cfg->domain_id, cfg->perf_level_khz);

		zassert_ok(cpu_freq_pstate_set(state),
			   "cpu_freq_pstate_set P-state[%d] failed", i);

		k_sleep(K_MSEC(SETTLE_MS));

		actual_khz = get_current_khz(cfg->domain_id);

		zassert_equal(actual_khz, cfg->perf_level_khz,
			      "P-state[%d]: set %u kHz but SCMI platform reports %u kHz",
			      i, cfg->perf_level_khz, actual_khz);

		LOG_INF("P-state[%d]: %u MHz confirmed by SCMI platform",
			i, actual_khz / 1000U);
	}
}

ZTEST(scmi_cpu_freq_basic, test_on_demand_policy_load_driven)
{
	const struct pstate *pstate_high_load;
	const struct pstate *pstate_low_load;
	int ret;

	/* NULL guard */
	zassert_equal(cpu_freq_policy_select_pstate(NULL), -EINVAL,
		      "Expected -EINVAL for NULL pstate_out");

	/* High-load scenario */
	k_busy_wait(BUSY_WAIT_US);
	ret = cpu_freq_policy_select_pstate(&pstate_high_load);
	zassert_ok(ret, "policy_select_pstate (high load) failed: %d", ret);
	LOG_INF("High load → P-state threshold=%u%%",
		pstate_high_load->load_threshold);

	/* Low-load scenario */
	k_sleep(K_MSEC(SLEEP_MS));
	ret = cpu_freq_policy_select_pstate(&pstate_low_load);
	zassert_ok(ret, "policy_select_pstate (low load) failed: %d", ret);
	LOG_INF("Low load  → P-state threshold=%u%%",
		pstate_low_load->load_threshold);

	/* Policy must select a lower pstate after idle */
	zassert_true(
		pstate_low_load->load_threshold <
		pstate_high_load->load_threshold,
		"Expected lower threshold after sleep: high=%u%% low=%u%%",
		pstate_high_load->load_threshold,
		pstate_low_load->load_threshold);
}

ZTEST_SUITE(scmi_cpu_freq_basic, NULL, NULL, NULL, NULL, NULL);
