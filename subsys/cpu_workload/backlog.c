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

LOG_MODULE_REGISTER(cpu_workload, CONFIG_CPU_WORKLOAD_LOG_LEVEL);

#define CPU_WORKLOAD_BACKLOG_EMPTY_CONFIDENCE 100U
#define CPU_WORKLOAD_BACKLOG_FALLBACK_CONFIDENCE 10U

struct ready_backlog_context {
	struct cpu_workload_ready_backlog_sample *sample;
	uint8_t confidence;
};

static bool cpu_workload_thread_is_runnable(const struct k_thread *thread)
{
	if ((thread->base.thread_state & _THREAD_QUEUED) != 0) {
		return true;
	}

#ifdef CONFIG_SMP
	if (thread->base.cpu >= arch_num_cpus()) {
		return false;
	}

	return (_kernel.cpus[thread->base.cpu].current == thread) &&
		(_kernel.cpus[thread->base.cpu].idle_thread != thread);
#else
	return false;
#endif /* CONFIG_SMP */
}

static void cpu_workload_ready_backlog_cb(const struct k_thread *thread, void *user_data)
{
	struct ready_backlog_context *ctx = user_data;
	struct k_thread_runtime_cycles_profile profile;
	int ret;

	if (!cpu_workload_thread_is_runnable(thread)) {
		return;
	}

	ctx->sample->runnable_threads++;

	ret = k_thread_runtime_cycles_profile_get((k_tid_t)thread, &profile);
	if ((ret == 0) && (profile.sample_count > 0U) &&
	    (profile.confidence >= CONFIG_CPU_WORKLOAD_READY_BACKLOG_MIN_CONFIDENCE)) {
		ctx->sample->ready_backlog_cycles += profile.burst_avg_cycles;
		ctx->sample->profiled_threads++;
		ctx->sample->source_mask |= CPU_WORKLOAD_SOURCE_THREAD_BURST_PROFILE;
		ctx->confidence = MIN(ctx->confidence, profile.confidence);
		return;
	}

	if (CONFIG_CPU_WORKLOAD_READY_BACKLOG_FALLBACK_CYCLES > 0) {
		ctx->sample->ready_backlog_cycles +=
			CONFIG_CPU_WORKLOAD_READY_BACKLOG_FALLBACK_CYCLES;
		ctx->confidence = MIN(ctx->confidence, CPU_WORKLOAD_BACKLOG_FALLBACK_CONFIDENCE);
	} else {
		ctx->confidence = 0U;
	}
}

int cpu_workload_ready_backlog_get(int cpu_id,
					   struct cpu_workload_ready_backlog_sample *sample)
{
	if ((sample == NULL) || (cpu_id < 0) || (cpu_id >= arch_num_cpus())) {
		return -EINVAL;
	}

	struct ready_backlog_context ctx = {
		.sample = sample,
		.confidence = UINT8_MAX,
	};

	sample->ready_backlog_cycles = 0U;
	sample->source_mask = CPU_WORKLOAD_SOURCE_READY_BACKLOG;
	sample->runnable_threads = 0U;
	sample->profiled_threads = 0U;
	sample->confidence = 0U;

	k_thread_foreach_filter_by_cpu((unsigned int)cpu_id, cpu_workload_ready_backlog_cb, &ctx);

	if (sample->runnable_threads == 0U) {
		sample->confidence = CPU_WORKLOAD_BACKLOG_EMPTY_CONFIDENCE;
	} else if (ctx.confidence != UINT8_MAX) {
		sample->confidence = ctx.confidence;
	}

	LOG_DBG("CPU%d ready backlog: cycles=%llu runnable=%u profiled=%u confidence=%u",
		cpu_id, (unsigned long long)sample->ready_backlog_cycles,
		sample->runnable_threads, sample->profiled_threads, sample->confidence);

	return 0;
}
