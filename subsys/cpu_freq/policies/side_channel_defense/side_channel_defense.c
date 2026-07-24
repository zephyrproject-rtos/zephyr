/*
 * Copyright (c) 2026 Sean Kyer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(cpu_freq_policy_side_channel_defense, CONFIG_CPU_FREQ_LOG_LEVEL);

const struct pstate *soc_pstates[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(performance_states), PSTATE_DT_GET, (,))
};

const size_t soc_pstates_count = ARRAY_SIZE(soc_pstates);

int cpu_freq_policy_select_pstate(const struct pstate **pstate_out)
{
	size_t idx;
	uint32_t rnd;

	if (pstate_out == NULL) {
		return -EINVAL;
	}

	rnd = sys_rand32_get();

	/* Uniform mapping to valid P-state range */
	idx = (uint32_t)(((uint64_t)rnd * soc_pstates_count) >> 32);

	*pstate_out = soc_pstates[idx];

	LOG_DBG("RNG policy: rnd=%u idx=%zu freq_level=%d", rnd, idx,
		soc_pstates[idx]->load_threshold);

	return 0;
}

void cpu_freq_policy_reset(void)
{
    /* No reset required */
}

const struct pstate *cpu_freq_policy_pstate_set(const struct pstate *state)
{
	int rv = cpu_freq_pstate_set(state);

	if (rv != 0) {
		LOG_ERR("Failed to set P-state: %d", rv);
		return NULL;
	}

	return state;
}
