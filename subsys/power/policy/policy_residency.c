/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include "pm_policy.h"

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

static const struct pm_state_info pm_min_residency[] =
	PM_STATE_INFO_DT_ITEMS_LIST(DT_NODELABEL(cpu0));

struct pm_state_info pm_policy_next_state(int32_t ticks)
{
	int i;

	for (i = ARRAY_SIZE(pm_min_residency) - 1; i >= 0; i--) {
		uint32_t min_residency, exit_latency;

		if (!pm_constraint_get(pm_min_residency[i].state)) {
			continue;
		}

		min_residency = k_us_to_ticks_ceil32(
			    pm_min_residency[i].min_residency_us);
		exit_latency = k_us_to_ticks_ceil32(
			    pm_min_residency[i].exit_latency_us);
		__ASSERT(min_residency > exit_latency,
				"min_residency_us < exit_latency_us");

		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= (min_residency - exit_latency))) {
			LOG_DBG("Selected power state %d "
				"(ticks: %d, min_residency: %u)",
				pm_min_residency[i].state, ticks,
				pm_min_residency[i].min_residency_us);
			return pm_min_residency[i];
		}
	}

	LOG_DBG("No suitable power state found!");
	return (struct pm_state_info){PM_STATE_ACTIVE, 0, 0};
}
