/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/policy.h>
#include <zephyr/sys_clock.h>
#include <zephyr/pm/device.h>

extern struct pm_policy_event *next_event;
extern int32_t max_latency_cyc;

const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	int64_t cyc = -1;
	uint8_t num_cpu_states;
	const struct pm_state_info *cpu_states;

#ifdef CONFIG_PM_NEED_ALL_DEVICES_IDLE
	if (pm_device_is_any_busy()) {
		return NULL;
	}
#endif

	if (ticks != K_TICKS_FOREVER) {
		cyc = k_ticks_to_cyc_ceil32(ticks);
	}

	num_cpu_states = pm_state_cpu_get_all(cpu, &cpu_states);

	if ((next_event) && (next_event->value_cyc >= 0)) {
		uint32_t cyc_curr = k_cycle_get_32();
		int64_t cyc_evt = next_event->value_cyc - cyc_curr;

		/* event happening after cycle counter max value, pad */
		if (next_event->value_cyc <= cyc_curr) {
			cyc_evt += UINT32_MAX;
		}

		if (cyc_evt > 0) {
			/* if there's no system wakeup event always wins,
			 * otherwise, who comes earlier wins
			 */
			if (cyc < 0) {
				cyc = cyc_evt;
			} else {
				cyc = MIN(cyc, cyc_evt);
			}
		}
	}

	for (int16_t i = (int16_t)num_cpu_states - 1; i >= 0; i--) {
		const struct pm_state_info *state = &cpu_states[i];
		uint32_t min_residency_cyc, exit_latency_cyc;

		/* check if there is a lock on state + substate */
		if (pm_policy_state_lock_is_active(state->state, state->substate_id)) {
			continue;
		}

		min_residency_cyc = k_us_to_cyc_ceil32(state->min_residency_us);
		exit_latency_cyc = k_us_to_cyc_ceil32(state->exit_latency_us);

		/* skip state if it brings too much latency */
		if ((max_latency_cyc >= 0) &&
		    (exit_latency_cyc >= max_latency_cyc)) {
			continue;
		}

		if ((cyc < 0) ||
		    (cyc >= (min_residency_cyc + exit_latency_cyc))) {
			return state;
		}
	}

	return NULL;
}
