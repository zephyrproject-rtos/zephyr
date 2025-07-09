/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/cpu_load/cpu_load.h>

LOG_MODULE_REGISTER(cpu_freq_policy_on_demand, CONFIG_CPU_FREQ_LOG_LEVEL);

const struct pstate *soc_pstates[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(performance_states), PSTATE_DT_GET, (,))
};

/*
 * On-demand policy scans the list of P-states from the devicetree and selects the
 * first P-state where the cpu_load is greater than or equal to the trigger threshold
 * of the P-state.
 */
int cpu_freq_policy_select_pstate(const struct pstate **pstate_out)
{
	int cpu_load;

	if (pstate_out == NULL) {
		LOG_ERR("On-Demand Policy: pstate_out is NULL");
		return -EINVAL;
	}

	cpu_load = cpu_load_get(0);
	if (cpu_load < 0) {
		LOG_ERR("Unable to retrieve CPU load");
		return cpu_load;
	}

	LOG_DBG("Current CPU Load: %d%%", cpu_load);

	for (int i = 0; i < ARRAY_SIZE(soc_pstates); i++) {
		const struct pstate *state = soc_pstates[i];

		if (cpu_load >= state->load_threshold) {
			*pstate_out = state;
			LOG_DBG("On-Demand Policy: Selected P-state %d with load_threshold=%d%%", i,
				state->load_threshold);
			return 0;
		}
	}

	LOG_ERR("On-Demand Policy: No suitable P-state found for CPU load %d%%", cpu_load);

	return -ENOTSUP;
}
