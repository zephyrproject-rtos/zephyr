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

#include "cpu_workload.h"

LOG_MODULE_REGISTER(cpu_workload, CONFIG_CPU_WORKLOAD_LOG_LEVEL);

#define CPU_WORKLOAD_BACKLOG_EMPTY_CONFIDENCE 100U

struct ready_backlog_context {
	struct cpu_workload_ready_backlog *backlog;
	uint8_t confidence;
};

static void cpu_workload_ready_backlog_cb(const struct k_thread *thread, void *user_data)
{
	struct ready_backlog_context *ctx = user_data;
	struct cpu_workload_thread_burst_profile profile;
	bool runnable;
	int ret;

	runnable = (thread->base.thread_state & _THREAD_QUEUED) != 0;
	if (!runnable) {
		return;
	}

	ctx->backlog->runnable_threads++;

	ret = cpu_workload_thread_burst_profile_get((k_tid_t)thread, &profile);

	/* After obtaining the EWMA burst profile for this thread, we only trust it if
	 * the profile is mature (has samples + confidence exceeds threshold).
	 *
	 * 1. add its burst_avg_cycles to the total backlog.
	 * 2. records the number of threads contributing based on actual profiling.
	 * 3. tells the caller this result contains thread burst profiling data.
	 * 4. barrel effect: the overall confidence is determined by the worst contributor.
	 */
	if ((ret == 0) && (profile.sample_count > 0U) &&
	    (profile.confidence >= CONFIG_CPU_WORKLOAD_READY_BACKLOG_MIN_CONFIDENCE)) {
		ctx->backlog->ready_backlog_cycles += profile.burst_avg_cycles;
		ctx->backlog->profiled_threads++;
		ctx->backlog->source_mask |= CPU_WORKLOAD_SOURCE_THREAD_BURST_PROFILE;
		ctx->confidence = MIN(ctx->confidence, profile.confidence);
		return;
	}

	ctx->confidence = 0U;
}

int cpu_workload_ready_backlog_get(int cpu_id, struct cpu_workload_ready_backlog *backlog)
{
	CHECKIF((backlog == NULL) || (cpu_id < 0) || (cpu_id >= arch_num_cpus())) {
		return -EINVAL;
	}

	struct ready_backlog_context ctx = {
		.backlog = backlog,
		.confidence = UINT8_MAX,
	};

	backlog->ready_backlog_cycles = 0U;
	backlog->source_mask = CPU_WORKLOAD_SOURCE_READY_BACKLOG;
	backlog->runnable_threads = 0U;
	backlog->profiled_threads = 0U;
	backlog->confidence = 0U;

	k_thread_foreach_filter_by_cpu((unsigned int)cpu_id, cpu_workload_ready_backlog_cb, &ctx);

	if (backlog->runnable_threads == 0U) {
		backlog->confidence = CPU_WORKLOAD_BACKLOG_EMPTY_CONFIDENCE;
	} else if (ctx.confidence != UINT8_MAX) {
		backlog->confidence = ctx.confidence;
	}

	LOG_DBG("CPU%d ready backlog: cycles=%llu runnable=%u profiled=%u confidence=%u",
		cpu_id, (unsigned long long)backlog->ready_backlog_cycles,
		backlog->runnable_threads, backlog->profiled_threads, backlog->confidence);

	return 0;
}
