/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cpu_load_metric, CONFIG_CPU_LOAD_LOG_LEVEL);

static uint64_t execution_cycles_prev[CONFIG_MP_MAX_NUM_CPUS];
static uint64_t total_cycles_prev[CONFIG_MP_MAX_NUM_CPUS];

static struct k_spinlock lock[CONFIG_MP_MAX_NUM_CPUS];

int cpu_load_metric_get(int cpu_id)
{
	int ret;
	int load;
	uint64_t execution_cycles;
	uint64_t total_cycles;
	k_spinlock_key_t key;

	struct k_thread_runtime_stats cpu_query;

	ret = k_thread_runtime_stats_cpu_get(cpu_id, &cpu_query);
	if (ret) {
		LOG_ERR("Could not retrieve runtime statistics from scheduler");
		return ret;
	}

	key = k_spin_lock(&lock[cpu_id]);

	execution_cycles = cpu_query.execution_cycles - execution_cycles_prev[cpu_id];
	total_cycles = cpu_query.total_cycles - total_cycles_prev[cpu_id];

	execution_cycles_prev[cpu_id] = cpu_query.execution_cycles;
	total_cycles_prev[cpu_id] = cpu_query.total_cycles;

	k_spin_unlock(&lock[cpu_id], key);

	LOG_DBG("CPU%d Execution cycles: %llu, Total cycles: %llu",
		cpu_id, execution_cycles, total_cycles);

	if (execution_cycles == 0) {
		load = 0;
	} else {
		load = (int)((100 * total_cycles) / execution_cycles);
	}

	return load;
}
