/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys_clock.h>

const struct pm_state_info *pm_policy_state_for_residency(uint8_t cpu,
							  uint64_t idle_time_us)
{
	uint64_t ticks64 = k_us_to_ticks_ceil64(idle_time_us);
	int32_t ticks = (ticks64 > (uint64_t)INT32_MAX) ? K_TICKS_FOREVER : (int32_t)ticks64;

	return pm_policy_next_state(cpu, ticks);
}
