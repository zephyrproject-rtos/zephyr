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

LOG_MODULE_REGISTER(native_sim_cpu_freq, CONFIG_CPU_FREQ_LOG_LEVEL);

struct native_sim_config {
	int state_id;
};

int cpu_freq_pstate_set(const struct pstate *state)
{
	if (state == NULL) {
		LOG_ERR("pstate is NULL");
		return -EINVAL;
	}

	int state_id = ((const struct native_sim_config *)state->config)->state_id;

	LOG_DBG("Setting performance state: %d", state_id);

	switch (state_id) {
	case 0:
		LOG_DBG("Setting P-state 0: Nominal Mode");
		break;
	case 1:
		LOG_DBG("Setting P-state 1: Low Power Mode");
		break;
	case 2:
		LOG_DBG("Setting P-state 2: Ultra-low Power Mode");
		break;
	default:
		LOG_ERR("Unsupported P-state: %d", state_id);
		return -1;
	}

	return 0;
}

#define DEFINE_NATIVE_SIM_CONFIG(node_id)                                                          \
	static const struct native_sim_config _CONCAT(native_sim_config_, node_id) = {             \
		.state_id = DT_PROP(node_id, pstate_id),                                           \
	};                                                                                         \
	PSTATE_DT_DEFINE(node_id, &_CONCAT(native_sim_config_, node_id))

DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(performance_states), DEFINE_NATIVE_SIM_CONFIG)
