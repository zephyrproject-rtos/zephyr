/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pm/pm.h>
#include <pm/policy.h>
#include <sys_clock.h>
#include <sys/__assert.h>
#include <sys/time_units.h>
#include <sys/atomic.h>
#include <toolchain.h>

/** State lock reference counting */
static atomic_t state_lock_cnt[PM_STATE_COUNT];

#ifdef CONFIG_PM_POLICY_DEFAULT
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	uint8_t num_cpu_states;
	const struct pm_state_info *cpu_states;

	num_cpu_states = pm_state_cpu_get_all(cpu, &cpu_states);

	for (int16_t i = (int16_t)num_cpu_states - 1; i >= 0; i--) {
		const struct pm_state_info *state = &cpu_states[i];
		uint32_t min_residency, exit_latency;

		if (pm_policy_state_lock_is_active(state->state)) {
			continue;
		}

		min_residency = k_us_to_ticks_ceil32(state->min_residency_us);
		exit_latency = k_us_to_ticks_ceil32(state->exit_latency_us);

		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= (min_residency + exit_latency))) {
			return state;
		}
	}

	return NULL;
}
#endif

void pm_policy_state_lock_get(enum pm_state state)
{
	atomic_inc(&state_lock_cnt[state]);
}

void pm_policy_state_lock_put(enum pm_state state)
{
	atomic_t cnt = atomic_dec(&state_lock_cnt[state]);

	ARG_UNUSED(cnt);

	__ASSERT(cnt >= 1, "Unbalanced state lock get/put");
}

bool pm_policy_state_lock_is_active(enum pm_state state)
{
	return (atomic_get(&state_lock_cnt[state]) != 0);
}
