/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cpu_workload/cpu_workload.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>

#include "cpu_workload_internal.h"

LOG_MODULE_REGISTER(cpu_workload_arrival, CONFIG_CPU_WORKLOAD_LOG_LEVEL);

struct arrival_context {
	struct cpu_workload_arrival *arrival;
	uint8_t confidence; /* Global minimum confidence maintained during traversal. */
};

/**
 * Translate the raw source from the kernel layer thread arrival statistics into
 * the publicly exposed source mask at the cpu_workload layer.
 */
static uint32_t cpu_workload_arrival_source_mask(uint32_t source_mask)
{
	uint32_t workload_mask = 0U;

	if ((source_mask & K_THREAD_ARRIVAL_SOURCE_TIMEOUT) != 0U) {
		workload_mask |= CPU_WORKLOAD_SOURCE_ARRIVAL_TIMEOUT;
	}

	if ((source_mask & K_THREAD_ARRIVAL_SOURCE_SYNC) != 0U) {
		workload_mask |= CPU_WORKLOAD_SOURCE_ARRIVAL_SYNC;
	}

	if ((source_mask & K_THREAD_ARRIVAL_SOURCE_EXPLICIT) != 0U) {
		workload_mask |= CPU_WORKLOAD_SOURCE_ARRIVAL_EXPLICIT;
	}

	return workload_mask;
}

static void cpu_workload_arrival_cb(const struct k_thread *thread, void *user_data)
{
	struct cpu_workload_thread_burst_profile burst_profile;
	struct arrival_context *ctx = user_data;
	k_thread_arrival_stats_t stats;
	int ret;

	/* Read the thread's arrival stats and reset the thread's arrival window. */
	ret = k_thread_arrival_stats_get((k_tid_t)thread, &stats, true);
	if ((ret != 0) || (stats.count == 0U)) {
		return;
	}

	ctx->arrival->source_mask |= CPU_WORKLOAD_SOURCE_ARRIVAL |
		cpu_workload_arrival_source_mask(stats.source_mask);
	ctx->arrival->arrivals += stats.count;

	/* Get thread burst profile to estimate expected arrival cycles.
	 *
	 * If the burst profile cannot be obtained, or if this profile doesn't have samples yet,
	 * then the arrival count is still included in the total, but this portion of arrivals
	 * cannot be converted to expected_arrival_cycles, and it pushes the global confidence
	 * down to 0.
	 */
	ret = cpu_workload_thread_burst_profile_get((k_tid_t)thread, &burst_profile);
	if ((ret != 0) || (burst_profile.sample_count == 0U)) {
		ctx->confidence = 0U;
		return;
	}

	/* Future workload for this thread:
	 *	stats.count recent arrivals * predicted_cycles per arrival.
	 */
	ctx->arrival->expected_arrival_cycles +=
		(uint64_t)burst_profile.burst_avg_cycles * stats.count;
	ctx->arrival->profiled_arrivals += stats.count;

	/* Using a conservative strategy (weakest link in the chain), among the estimates
	 * for this batch of arrival workload, the least reliable one determines the overall
	 * reliability.
	 */
	if (ctx->confidence == UINT8_MAX) {
		ctx->confidence = burst_profile.confidence;
	} else if (ctx->confidence != 0U) {
		ctx->confidence = MIN(ctx->confidence, burst_profile.confidence);
	}
}

int cpu_workload_arrival_get(int cpu_id, struct cpu_workload_arrival *arrival)
{
	CHECKIF((arrival == NULL) || (cpu_id < 0) || (cpu_id >= arch_num_cpus())) {
		return -EINVAL;
	}

	struct arrival_context ctx = {
		.arrival = arrival,
		.confidence = UINT8_MAX,
	};

	arrival->expected_arrival_cycles = 0U;
	arrival->source_mask = 0U;
	arrival->arrivals = 0U;
	arrival->profiled_arrivals = 0U;
	arrival->confidence = 0U;

	/* Traverse all threads that are currently attributed to the specified CPU and
	 * accumulate arrival stats for those threads to produce the CPU's arrival workload.
	 */
	k_thread_foreach_filter_by_cpu((unsigned int)cpu_id, cpu_workload_arrival_cb, &ctx);

	if (ctx.confidence != UINT8_MAX) {
		arrival->confidence = ctx.confidence;
	}

	LOG_DBG("CPU%d arrivals: cycles=%llu arrivals=%u profiled=%u confidence=%u",
		cpu_id, (unsigned long long)arrival->expected_arrival_cycles,
		arrival->arrivals, arrival->profiled_arrivals, arrival->confidence);

	return 0;
}
