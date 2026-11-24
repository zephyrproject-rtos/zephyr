/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/power/ambiq_power.h>

#include <soc.h>

LOG_MODULE_REGISTER(apollo5x_cpu_freq, CONFIG_CPU_FREQ_LOG_LEVEL);

struct apollo5x_pstate_config {
	uint32_t mode;
};

int cpu_freq_pstate_set(const struct pstate *state)
{
	const struct apollo5x_pstate_config *cfg;
	int ret;

	if ((state == NULL) || (state->config == NULL)) {
		return -EINVAL;
	}

	cfg = state->config;
	ret = apollo5x_set_performance_mode(cfg->mode);
	if (ret) {
		return ret;
	}

	return 0;
}

#define APOLLO5X_PSTATE_DEFINE(node_id) \
	static const struct apollo5x_pstate_config \
	apollo5x_pstate_cfg_##node_id = { \
		.mode = DT_PROP(node_id, ambiq_perf_mode), \
	}; \
	PSTATE_DT_DEFINE(node_id, &apollo5x_pstate_cfg_##node_id)

DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(performance_states), APOLLO5X_PSTATE_DEFINE)
