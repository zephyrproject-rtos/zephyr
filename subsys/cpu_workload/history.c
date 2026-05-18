/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/cpu_workload/cpu_workload.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>

#include "cpu_workload_internal.h"

#define CPU_WORKLOAD_HISTORY_CONFIDENCE 100U

struct cpu_workload_history_state {
	/* Cumulative CPU execution cycles observed by the previous query. */
	uint64_t observed_execution_cycles;

	/* Cumulative CPU non-idle cycles observed by the previous query. */
	uint64_t observed_non_idle_cycles;

	/* Whether this CPU already has a baseline for delta calculation. */
	bool initialized;
};

static struct k_spinlock history_lock;
static struct cpu_workload_history_state history_states[CONFIG_MP_MAX_NUM_CPUS];

static uint8_t cpu_workload_history_load(uint64_t non_idle_cycles, uint64_t window_cycles)
{
	if (window_cycles == 0U) {
		return 0U;
	}

	if (non_idle_cycles >= window_cycles) {
		return 100U;
	}

	if (non_idle_cycles <= (UINT64_MAX / 100U)) {
		return (uint8_t)((non_idle_cycles * 100U) / window_cycles);
	}

	return (uint8_t)MIN(non_idle_cycles / (window_cycles / 100U), 100U);
}

static uint32_t cpu_workload_history_window_us(uint64_t window_cycles)
{
	uint64_t window_us = k_cyc_to_us_floor64(window_cycles);

	return (uint32_t)MIN(window_us, (uint64_t)UINT32_MAX);
}

int cpu_workload_history_get(int cpu_id, struct cpu_workload_history *history)
{
	struct cpu_workload_history_state *state;
	k_thread_runtime_stats_t stats;
	k_spinlock_key_t key;
	uint64_t window_cycles;
	uint64_t non_idle_cycles;
	int ret;

	CHECKIF((history == NULL) || (cpu_id < 0) || (cpu_id >= arch_num_cpus())) {
		return -EINVAL;
	}

	*history = (struct cpu_workload_history){};

	ret = k_thread_runtime_stats_cpu_get(cpu_id, &stats);
	if (ret != 0) {
		return ret;
	}

	key = k_spin_lock(&history_lock);
	state = &history_states[cpu_id];

	if (!state->initialized) {
		state->observed_execution_cycles = stats.execution_cycles;
		state->observed_non_idle_cycles = stats.total_cycles;
		state->initialized = true;
		k_spin_unlock(&history_lock, key);
		return -EAGAIN;
	}

	if ((stats.execution_cycles < state->observed_execution_cycles) ||
	    (stats.total_cycles < state->observed_non_idle_cycles)) {
		state->observed_execution_cycles = stats.execution_cycles;
		state->observed_non_idle_cycles = stats.total_cycles;
		k_spin_unlock(&history_lock, key);
		return -EAGAIN;
	}

	window_cycles = stats.execution_cycles - state->observed_execution_cycles;
	non_idle_cycles = stats.total_cycles - state->observed_non_idle_cycles;

	state->observed_execution_cycles = stats.execution_cycles;
	state->observed_non_idle_cycles = stats.total_cycles;

	k_spin_unlock(&history_lock, key);

	if (window_cycles == 0U) {
		return -EAGAIN;
	}

	history->non_idle_cycles = non_idle_cycles;
	history->window_cycles = window_cycles;
	history->window_us = cpu_workload_history_window_us(window_cycles);
	history->load = cpu_workload_history_load(non_idle_cycles, window_cycles);
	history->confidence = CPU_WORKLOAD_HISTORY_CONFIDENCE;

	return 0;
}
