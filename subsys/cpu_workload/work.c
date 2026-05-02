/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cpu_workload/cpu_workload.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cpu_workload_work, CONFIG_CPU_WORKLOAD_LOG_LEVEL);

int cpu_workload_work_get(struct k_work *work, struct cpu_workload_work_sample *sample)
{
	k_work_runtime_cycles_profile_t profile;
	int ret;

	if ((work == NULL) || (sample == NULL)) {
		return -EINVAL;
	}

	ret = k_work_runtime_cycles_profile_get(work, &profile);
	if (ret != 0) {
		return ret;
	}

	sample->handler_cycles = profile.handler_avg_cycles;
	sample->source_mask = (profile.sample_count == 0U) ? 0U : CPU_WORKLOAD_SOURCE_WORK_PROFILE;
	sample->sample_count = profile.sample_count;
	sample->confidence = profile.confidence;

	LOG_DBG("work %p handler profile: cycles=%u samples=%u confidence=%u",
		work, sample->handler_cycles, sample->sample_count, sample->confidence);

	return 0;
}
