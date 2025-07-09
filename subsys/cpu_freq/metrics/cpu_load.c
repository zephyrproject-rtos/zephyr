/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cpu_freq_metrics_cpu_load, CONFIG_CPU_FREQ_LOG_LEVEL);

static uint64_t execution_cycles_prev;
static uint64_t total_cycles_prev;

int cpu_freq_metric_get_cpu_load(uint32_t *load)
{
	int ret;
	uint64_t execution_cycles;
	uint64_t total_cycles;

	struct k_thread_runtime_stats cpu_query;

	ret = k_thread_runtime_stats_cpu_get(0, &cpu_query);
	if (ret) {
		LOG_ERR("Could not retrieve runtime statistics from scheduler");
		return ret;
	}

	execution_cycles = cpu_query.execution_cycles - execution_cycles_prev;
	total_cycles = cpu_query.total_cycles - total_cycles_prev;

	LOG_DBG("Execution cycles: %llu, Total cycles: %llu", execution_cycles, total_cycles);

	*load = (uint32_t)((100 * total_cycles) / execution_cycles);

	execution_cycles_prev = cpu_query.execution_cycles;
	total_cycles_prev = cpu_query.total_cycles;

	return 0;
}
