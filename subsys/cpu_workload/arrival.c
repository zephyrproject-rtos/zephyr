/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cpu_workload/cpu_workload.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(cpu_workload_arrival, CONFIG_CPU_WORKLOAD_LOG_LEVEL);

struct arrival_context {
	struct cpu_workload_arrival_sample *sample;
	uint8_t confidence;
};

static void cpu_workload_arrival_cb(const struct k_thread *thread, void *user_data)
{
	struct arrival_context *ctx = user_data;
	k_thread_workload_arrival_profile_t profile;
	int ret;

	ret = k_thread_workload_arrival_profile_get((k_tid_t)thread, &profile, true);
	if ((ret != 0) || (profile.arrival_count == 0U)) {
		return;
	}

	ctx->sample->expected_arrival_cycles += profile.expected_arrival_cycles;
	ctx->sample->source_mask |= profile.source_mask;
	ctx->sample->arrivals += profile.arrival_count;
	ctx->sample->profiled_arrivals += profile.profiled_arrivals;

	if (profile.profiled_arrivals == 0U) {
		ctx->confidence = 0U;
	} else if (ctx->confidence == UINT8_MAX) {
		ctx->confidence = profile.confidence;
	} else if (ctx->confidence != 0U) {
		ctx->confidence = MIN(ctx->confidence, profile.confidence);
	}
}

int cpu_workload_arrival_get(int cpu_id, struct cpu_workload_arrival_sample *sample)
{
	struct arrival_context ctx;

	if ((sample == NULL) || (cpu_id < 0) || (cpu_id >= arch_num_cpus())) {
		return -EINVAL;
	}

	sample->expected_arrival_cycles = 0U;
	sample->source_mask = CPU_WORKLOAD_SOURCE_ARRIVAL;
	sample->arrivals = 0U;
	sample->profiled_arrivals = 0U;
	sample->confidence = 0U;

	ctx.sample = sample;
	ctx.confidence = UINT8_MAX;

	k_thread_foreach_filter_by_cpu((unsigned int)cpu_id, cpu_workload_arrival_cb, &ctx);

	if (sample->arrivals == 0U) {
		sample->source_mask = 0U;
	} else if (ctx.confidence != UINT8_MAX) {
		sample->confidence = ctx.confidence;
	} else {
		sample->confidence = 0U;
	}

	LOG_DBG("CPU%d arrivals: cycles=%llu arrivals=%u profiled=%u confidence=%u",
		cpu_id, (unsigned long long)sample->expected_arrival_cycles,
		sample->arrivals, sample->profiled_arrivals, sample->confidence);

	return 0;
}