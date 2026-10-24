/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Dummy CPU provider for the initial performance counter session publication.
 *
 * This provider exercises event discovery and session control without accessing
 * hardware counters. The cpu0.cycles event returns zero for both counter snapshots;
 * it does not measure CPU cycles. Hardware-backed counting is not implemented here.
 */

#include <errno.h>
#include <string.h>

#include "../provider.h"

#define CPU_EVENT_CYCLES 0U

static int cpu_provider_list(void *context, perf_event_list_cb_t callback, void *user_data)
{
	ARG_UNUSED(context);

	if (callback == NULL) {
		return -EINVAL;
	}

	return callback("cycles", "CPU cycle counter", user_data);
}

static int cpu_provider_lookup(void *context, const char *event_name, uint64_t *event_id)
{
	ARG_UNUSED(context);

	if (event_name == NULL || event_id == NULL) {
		return -EINVAL;
	}
	if (strcmp(event_name, "cycles") != 0) {
		return -ENOENT;
	}

	*event_id = CPU_EVENT_CYCLES;
	return 0;
}

static int cpu_provider_prepare(void *context, uint64_t event_id)
{
	ARG_UNUSED(context);

	return event_id == CPU_EVENT_CYCLES ? 0 : -EINVAL;
}

static int cpu_provider_start(void *context, uint64_t event_id, uint64_t *baseline)
{
	ARG_UNUSED(context);

	if (event_id != CPU_EVENT_CYCLES || baseline == NULL) {
		return -EINVAL;
	}

	*baseline = 0U;
	return 0;
}

static int cpu_provider_stop(void *context, uint64_t event_id, uint64_t *final_count)
{
	ARG_UNUSED(context);

	if (event_id != CPU_EVENT_CYCLES || final_count == NULL) {
		return -EINVAL;
	}

	*final_count = 0U;
	return 0;
}

static const struct perf_event_provider_api cpu_provider_api = {
	.list = cpu_provider_list,
	.lookup = cpu_provider_lookup,
	.prepare = cpu_provider_prepare,
	.start = cpu_provider_start,
	.stop = cpu_provider_stop,
};

PERF_EVENT_PROVIDER_DEFINE(cpu0, "CPU performance counter provider", &cpu_provider_api, NULL);
