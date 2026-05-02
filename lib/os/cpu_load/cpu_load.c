/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/cpu_load.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(cpu_load_metric, CONFIG_CPU_LOAD_LOG_LEVEL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_THREAD_RUNTIME_STATS_USE_TIMING_FUNCTIONS),
	     "CPU load samples require CPU cycle runtime statistics");

#define CPU_LOAD_CONFIDENCE_BASE 40U
#define CPU_LOAD_CONFIDENCE_STEP 10U
#define CPU_LOAD_CONFIDENCE_MAX 80U

static uint64_t execution_cycles_prev[CONFIG_MP_MAX_NUM_CPUS];
static uint64_t total_cycles_prev[CONFIG_MP_MAX_NUM_CPUS];
static uint8_t sample_count[CONFIG_MP_MAX_NUM_CPUS];
static bool initialized[CONFIG_MP_MAX_NUM_CPUS];

static struct k_spinlock lock[CONFIG_MP_MAX_NUM_CPUS];

static uint8_t cpu_load_confidence(int cpu_id)
{
	uint32_t confidence = CPU_LOAD_CONFIDENCE_BASE +
		(sample_count[cpu_id] * CPU_LOAD_CONFIDENCE_STEP);

	return (uint8_t)MIN(confidence, CPU_LOAD_CONFIDENCE_MAX);
}

int cpu_load_sample_get(int cpu_id, struct cpu_load_sample *sample)
{
	int ret;
	k_spinlock_key_t key;
	uint64_t window_us;
	uint64_t total_cycles;
	uint64_t execution_cycles;
	struct k_thread_runtime_stats cpu_query;

	if ((sample == NULL) || (cpu_id < 0) || (cpu_id >= arch_num_cpus())) {
		return -EINVAL;
	}

	ret = k_thread_runtime_stats_cpu_get(cpu_id, &cpu_query);
	if (ret != 0) {
		LOG_ERR("Could not retrieve runtime statistics from scheduler");
		return ret;
	}

	key = k_spin_lock(&lock[cpu_id]);

	if (!initialized[cpu_id] ||
	    (cpu_query.execution_cycles < execution_cycles_prev[cpu_id]) ||
	    (cpu_query.total_cycles < total_cycles_prev[cpu_id])) {
		execution_cycles_prev[cpu_id] = cpu_query.execution_cycles;
		total_cycles_prev[cpu_id] = cpu_query.total_cycles;
		sample_count[cpu_id] = 0U;
		initialized[cpu_id] = true;
		k_spin_unlock(&lock[cpu_id], key);
		return -EAGAIN;
	}

	execution_cycles = cpu_query.execution_cycles - execution_cycles_prev[cpu_id];
	total_cycles = cpu_query.total_cycles - total_cycles_prev[cpu_id];

	execution_cycles_prev[cpu_id] = cpu_query.execution_cycles;
	total_cycles_prev[cpu_id] = cpu_query.total_cycles;

	if ((execution_cycles == 0U) || (total_cycles > execution_cycles)) {
		k_spin_unlock(&lock[cpu_id], key);
		return -EAGAIN;
	}

	if (sample_count[cpu_id] < UINT8_MAX) {
		sample_count[cpu_id]++;
	}

	window_us = k_cyc_to_us_floor64(execution_cycles);

	sample->non_idle_cycles = total_cycles;
	sample->window_us = (uint32_t)MIN(window_us, (uint64_t)UINT32_MAX);
	sample->load = (uint8_t)((100U * total_cycles) / execution_cycles);
	sample->confidence = cpu_load_confidence(cpu_id);
	sample->source_mask = CPU_LOAD_SAMPLE_SOURCE_RUNTIME_HISTORY;

	k_spin_unlock(&lock[cpu_id], key);

	LOG_DBG("CPU%d Execution cycles: %llu, Total cycles: %llu",
		cpu_id, (unsigned long long)execution_cycles,
		(unsigned long long)total_cycles);

	return 0;
}

int cpu_load_metric_get(int cpu_id)
{
	struct cpu_load_sample sample;
	int ret;

	ret = cpu_load_sample_get(cpu_id, &sample);
	if (ret == -EAGAIN) {
		return 0;
	}
	if (ret != 0) {
		return ret;
	}

	return sample.load;
}
