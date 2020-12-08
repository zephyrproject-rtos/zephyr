/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <power/power_state.h>
#include "pm_policy.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(power, CONFIG_PM_LOG_LEVEL);

static const struct pm_state_info pm_dummy_states[] =
	PM_STATE_DT_ITEMS_LIST(DT_NODELABEL(cpu0));

enum pm_state pm_policy_next_state(int32_t ticks)
{
	static enum pm_state cur_power_state;
	int i = (int)cur_power_state;
	uint8_t states_len = ARRAY_SIZE(pm_dummy_states);

	if (states_len == 0) {
		/* No power states to go through. */
		return PM_STATE_ACTIVE;
	}

	do {
		i = (i + 1) % states_len;

#ifdef CONFIG_PM_STATE_LOCK
		if (!pm_ctrl_is_state_enabled(pm_dummy_states[i].state)) {
			continue;
		}
#endif
		cur_power_state = pm_dummy_states[i].state;

		LOG_DBG("Selected power state: %u", pm_dummy_states[i].state);
		return pm_dummy_states[i].state;
	} while (pm_dummy_states[i].state != cur_power_state);

	LOG_DBG("No suitable power state found!");
	return PM_STATE_ACTIVE;
}

__weak bool pm_policy_low_power_devices(enum pm_state state)
{
	return pm_is_sleep_state(state);
}
