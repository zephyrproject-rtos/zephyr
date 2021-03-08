/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <power/pm_policy.h>

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

static const struct pm_state_info pm_min_residency[] =
	PM_STATE_INFO_DT_ITEMS_LIST(DT_NODELABEL(cpu0));

static int residency_policy_next_state(struct pm_policy *policy,
			       int32_t ticks, struct pm_state_info *state)
{
	int i;

	ARG_UNUSED(policy);

	if (state == NULL) {
		return -EINVAL;
	}

	for (i = ARRAY_SIZE(pm_min_residency) - 1; i >= 0; i--) {
		if (!pm_constraint_get(pm_min_residency[i].state)) {
			continue;
		}

		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= k_us_to_ticks_ceil32(
			    pm_min_residency[i].min_residency_us))) {
			LOG_DBG("Selected power state %d "
				"(ticks: %d, min_residency: %u)",
				pm_min_residency[i].state, ticks,
				pm_min_residency[i].min_residency_us);
			*state = pm_min_residency[i];
			return 0;
		}
	}

	LOG_DBG("No suitable power state found!");
	*state = (struct pm_state_info){PM_STATE_ACTIVE, 0, 0};

	return 0;
}

PM_POLICY_DEFINE(residency_policy, residency_policy_next_state);
