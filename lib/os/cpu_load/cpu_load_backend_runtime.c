/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Scheduler runtime-statistics CPU load backend. The load is derived from the
 * per-CPU active and wall-clock cycle counters maintained by the scheduler.
 * Portable across architectures and supports multiple CPUs.
 */

#include <zephyr/sys/cpu_load.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(cpu_load, CONFIG_CPU_LOAD_LOG_LEVEL);

static uint64_t active_prev[CONFIG_MP_MAX_NUM_CPUS];
static uint64_t wall_prev[CONFIG_MP_MAX_NUM_CPUS];

static struct k_spinlock lock[CONFIG_MP_MAX_NUM_CPUS];

int cpu_load_get_cpu(unsigned int cpu_id, bool reset)
{
	int ret;
	uint64_t active;
	uint64_t wall;
	k_spinlock_key_t key;
	struct k_thread_runtime_stats stats;

	if (cpu_id >= CONFIG_MP_MAX_NUM_CPUS) {
		return -EINVAL;
	}

	ret = k_thread_runtime_stats_cpu_get((int)cpu_id, &stats);
	if (ret) {
		LOG_ERR("Could not retrieve runtime statistics from scheduler");
		return ret;
	}

	key = k_spin_lock(&lock[cpu_id]);

	/* total_cycles counts active (non-idle) cycles, execution_cycles counts
	 * the elapsed wall-clock cycles (active + idle).
	 */
	active = stats.total_cycles - active_prev[cpu_id];
	wall = stats.execution_cycles - wall_prev[cpu_id];

	if (reset) {
		active_prev[cpu_id] = stats.total_cycles;
		wall_prev[cpu_id] = stats.execution_cycles;
	}

	k_spin_unlock(&lock[cpu_id], key);

	LOG_DBG("CPU%u active cycles: %llu, wall cycles: %llu", cpu_id, active, wall);

	if (wall == 0) {
		return 0;
	}

	return (int)((1000 * active) / wall);
}
