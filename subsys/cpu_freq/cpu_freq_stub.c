/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stub_cpu_freq, CONFIG_CPU_FREQ_LOG_LEVEL);

struct stub_config {
	int state_id;
};

int cpu_freq_pstate_set(const struct pstate *state)
{
	if (state == NULL) {
		LOG_ERR("Stub pstate is NULL");
		return -EINVAL;
	}

	int state_id = ((const struct stub_config *)state->config)->state_id;

	LOG_DBG("Stub setting performance state: %d", state_id);

	switch (state_id) {
	case 0:
		LOG_DBG("Stub setting P-state 0: Nominal Mode\n");
		break;
	case 1:
		LOG_DBG("Stub setting P-state 1: Low Power Mode\n");
		break;
	case 2:
		LOG_DBG("Stub setting P-state 2: Ultra-low Power Mode\n");
		break;
	default:
		LOG_ERR("Stub unsupported P-state: %d", state_id);
		return -1;
	}

	return 0;
}

#define DEFINE_STUB_CONFIG(node_id)                             \
	static const struct stub_config                         \
		_CONCAT(stub_config_, node_id) = {              \
		.state_id = DT_PROP(node_id, pstate_id),        \
	};                                                      \
	PSTATE_DT_DEFINE(node_id, &_CONCAT(stub_config_, node_id))

DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(performance_states), DEFINE_STUB_CONFIG)
