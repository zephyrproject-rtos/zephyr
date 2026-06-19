/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Dynamic (timer-driven) SCMI cpu_freq test.
 *
 * The test generates high and low CPU load and verifies via
 * scmi_perf_level_get() that the scmi platform firmware actually changed the
 * CPU frequency in response.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/drivers/firmware/scmi/perf.h>

LOG_MODULE_REGISTER(scmi_cpu_freq_dynamic, LOG_LEVEL_INF);

/*
 * Duration of the warm-up busy phase before reading frequency.
 * With CONFIG_CPU_FREQ_INTERVAL_MS=50 the timer fires ~8 times during
 * 400 ms, giving the policy enough samples to reach the highest pstate.
 */
#define BUSY_WARMUP_MS   400U

/*
 * Duration of the tail busy phase after reading frequency.
 * Keeps the CPU busy while the SCMI read completes, preventing the
 * timer from triggering a frequency drop before the assertion.
 */
#define BUSY_TAIL_MS     100U

/*
 * Duration of the idle phase.  The timer fires ~10 times during 500 ms,
 * giving the policy enough samples to reach the lowest pstate.
 * Add one extra timer interval to ensure the last tick completes.
 */
#define IDLE_MS          600U

struct scmi_perf_pstate_cfg {
	uint32_t domain_id;
	uint32_t perf_level_khz;
};

extern const struct pstate *soc_pstates[];
extern const size_t soc_pstates_count;

static uint32_t get_current_khz(uint32_t domain_id)
{
	uint32_t khz = 0;

	zassert_ok(scmi_perf_level_get(domain_id, &khz),
		   "scmi_perf_level_get domain=%u failed", domain_id);
	return khz;
}

ZTEST(scmi_cpu_freq_dynamic, test_timer_driven)
{
	const struct scmi_perf_pstate_cfg *cfg_high =
		(const struct scmi_perf_pstate_cfg *)soc_pstates[0]->config;
	const struct scmi_perf_pstate_cfg *cfg_low =
		(const struct scmi_perf_pstate_cfg *)
		soc_pstates[soc_pstates_count - 1]->config;
	uint32_t domain_id = cfg_high->domain_id;
	uint32_t freq_after_busy, freq_after_idle;

	LOG_INF("Phase 1: busy-wait %u ms warm-up + %u ms tail "
		"(timer interval=%u ms)",
		BUSY_WARMUP_MS, BUSY_TAIL_MS, CONFIG_CPU_FREQ_INTERVAL_MS);

	k_busy_wait(BUSY_WARMUP_MS * USEC_PER_MSEC);

	freq_after_busy = get_current_khz(domain_id);
	LOG_INF("During busy: SCMI platform reports %u MHz", freq_after_busy / 1000U);

	k_busy_wait(BUSY_TAIL_MS * USEC_PER_MSEC);

	LOG_INF("Phase 2: idle sleep %u ms", IDLE_MS);

	k_sleep(K_MSEC(IDLE_MS));

	freq_after_idle = get_current_khz(domain_id);
	LOG_INF("After idle: SCMI platform reports %u MHz", freq_after_idle / 1000U);

	zassert_equal(freq_after_busy, cfg_high->perf_level_khz,
		      "Expected highest OPP %u kHz during busy, got %u kHz",
		      cfg_high->perf_level_khz, freq_after_busy);

	zassert_true(freq_after_idle < freq_after_busy,
		     "Expected lower freq after idle: "
		     "busy=%u kHz idle=%u kHz",
		     freq_after_busy, freq_after_idle);

	zassert_true(freq_after_idle <= cfg_low->perf_level_khz * 2U,
		     "Expected low freq after idle, got %u kHz "
		     "(lowest OPP=%u kHz)",
		     freq_after_idle, cfg_low->perf_level_khz);
}

ZTEST_SUITE(scmi_cpu_freq_dynamic, NULL, NULL, NULL, NULL, NULL);
