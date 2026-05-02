/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/cpu_workload/cpu_workload.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/cpu_load.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(cpu_workload_estimate, CONFIG_CPU_WORKLOAD_LOG_LEVEL);

static void confidence_merge(uint8_t *confidence, uint8_t next_confidence)
{
	if (*confidence == UINT8_MAX) {
		*confidence = next_confidence;
		return;
	}

	*confidence = MIN(*confidence, next_confidence);
}

int cpu_workload_estimate_get(int cpu_id, struct cpu_workload_estimate *estimate)
{
	struct cpu_workload_ready_backlog_sample backlog;
	struct cpu_workload_arrival_sample arrival;
	struct cpu_load_sample history;
	uint64_t forward_cycles;
	uint8_t confidence = UINT8_MAX;
	int ret;

	if ((estimate == NULL) || (cpu_id < 0) || (cpu_id >= arch_num_cpus())) {
		return -EINVAL;
	}

	estimate->estimated_cycles = 0U;
	estimate->ready_backlog_cycles = 0U;
	estimate->expected_arrival_cycles = 0U;
	estimate->history_cycles = 0U;
	estimate->history_window_us = 0U;
	estimate->source_mask = 0U;
	estimate->history_load = 0U;
	estimate->confidence = 0U;

	ret = cpu_workload_ready_backlog_get(cpu_id, &backlog);
	if (ret != 0) {
		return ret;
	}

	estimate->ready_backlog_cycles = backlog.ready_backlog_cycles;
	estimate->source_mask |= backlog.source_mask;
	confidence_merge(&confidence, backlog.confidence);

	ret = cpu_workload_arrival_get(cpu_id, &arrival);
	if (ret != 0) {
		return ret;
	}

	estimate->expected_arrival_cycles = arrival.expected_arrival_cycles;
	estimate->source_mask |= arrival.source_mask;
	if (arrival.arrivals > 0U) {
		confidence_merge(&confidence, arrival.confidence);
	}

	ret = cpu_load_sample_get(cpu_id, &history);
	if (ret == 0) {
		estimate->history_cycles = history.non_idle_cycles;
		estimate->history_window_us = history.window_us;
		estimate->history_load = history.load;
		estimate->source_mask |= CPU_WORKLOAD_SOURCE_RUNTIME_HISTORY;
		confidence_merge(&confidence, history.confidence);
	} else if (ret != -EAGAIN) {
		LOG_DBG("CPU%d runtime-history sample unavailable: %d", cpu_id, ret);
	}

	forward_cycles = estimate->ready_backlog_cycles + estimate->expected_arrival_cycles;
	estimate->estimated_cycles = MAX(forward_cycles, estimate->history_cycles);

	if (confidence != UINT8_MAX) {
		estimate->confidence = confidence;
	}

	LOG_DBG("CPU%d estimate: cycles=%llu forward=%llu history=%llu source=0x%x confidence=%u",
		cpu_id, (unsigned long long)estimate->estimated_cycles,
		(unsigned long long)forward_cycles, (unsigned long long)estimate->history_cycles,
		estimate->source_mask, estimate->confidence);

	return 0;
}