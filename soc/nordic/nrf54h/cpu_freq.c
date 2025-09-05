/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

struct pstate_config {
	const struct device *cpu_clock;
	uint32_t cpu_min_freq;
};

const struct pstate_config pstate0_config = {
	.cpu_clock = DEVICE_DT_GET(DT_PROP(DT_NODELABEL(pstate0), cpu_clock)),
	.cpu_min_freq = DT_PROP(DT_NODELABEL(pstate0), cpu_min_frequency),
};

const struct pstate_config pstate1_config = {
	.cpu_clock = DEVICE_DT_GET(DT_PROP(DT_NODELABEL(pstate1), cpu_clock)),
	.cpu_min_freq = DT_PROP(DT_NODELABEL(pstate1), cpu_min_frequency),
};

const struct pstate_config pstate2_config = {
	.cpu_clock = DEVICE_DT_GET(DT_PROP(DT_NODELABEL(pstate2), cpu_clock)),
	.cpu_min_freq = DT_PROP(DT_NODELABEL(pstate2), cpu_min_frequency),
};

PSTATE_DT_DEFINE(DT_NODELABEL(pstate0), &pstate0_config);
PSTATE_DT_DEFINE(DT_NODELABEL(pstate1), &pstate1_config);
PSTATE_DT_DEFINE(DT_NODELABEL(pstate2), &pstate2_config);

int32_t cpu_freq_pstate_set(const struct pstate *state)
{
	const struct pstate_config *config;
	struct nrf_clock_spec spec = {};

	if (state == NULL) {
		return -EINVAL;
	}

	config = state->config;
	spec.frequency = config->cpu_min_freq;
	return nrf_clock_control_request_sync(config->cpu_clock, &spec, K_SECONDS(1));
}
