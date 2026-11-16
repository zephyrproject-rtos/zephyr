/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pmu_helpers.h"

#include <zephyr/logging/log.h>
#include <errno.h>

LOG_MODULE_REGISTER(pmu_helpers, LOG_LEVEL_INF);

static bool pmu_test_ready;

bool pmu_ready(void)
{
	return pmu_test_ready;
}

bool pmu_pmuv3_bootstrap(int pmu_init_ret)
{
	pmu_test_ready = false;

	if (pmu_init_ret != 0) {
		LOG_INF("PMU unavailable (init failed: %d); benchmarks skipped", pmu_init_ret);
		return false;
	}

	if (perf_counter_num_counters() == 0U) {
		LOG_INF("PMU unavailable (no counters); benchmarks skipped");
		return false;
	}

	perf_counter_disable_all();

	if (perf_counter_config(0, PMU_EVT_INST_RETIRED) != 0) {
		LOG_INF("PMU unavailable (counter config failed); benchmarks skipped");
		return false;
	}

	perf_counter_enable(0);
	perf_counter_reset_all();
	perf_counter_cycle_reset();
	perf_counter_start();

	for (volatile int i = 0; i < 500000; i++) {
	}

	perf_counter_stop();

	if (perf_counter_cycle_count() == 0ULL || perf_counter_read(0) == 0ULL) {
		perf_counter_disable_all();
		LOG_INF("PMU counters did not advance; benchmarks skipped");
		return false;
	}

	perf_counter_disable_all();

	pmu_test_ready = true;
	LOG_INF("PMU counters verified: cycle and event counters advance");
	return true;
}

int pmu_configure_counters(const struct perf_counter_config *configs, uint32_t num_configs)
{
	uint32_t i;
	int ret;
	uint32_t n;

	if (!configs) {
		return -EINVAL;
	}

	n = perf_counter_num_counters();
	if (n == 0U) {
		return -ENODEV;
	}

	if (num_configs > n) {
		LOG_ERR("Requested %u counters, only %u available", num_configs, n);
		return -EINVAL;
	}

	perf_counter_disable_all();

	for (i = 0; i < num_configs; i++) {
		ret = perf_counter_config(i, configs[i].event);
		if (ret < 0) {
			return ret;
		}

		if (configs[i].enabled) {
			perf_counter_enable(i);
		}
	}

	return 0;
}
