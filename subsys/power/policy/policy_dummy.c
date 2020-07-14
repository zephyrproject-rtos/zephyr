/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include "pm_policy.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(power, CONFIG_SYS_PM_LOG_LEVEL);

enum power_states sys_pm_policy_next_state(int32_t ticks)
{
	static uint8_t cur_power_state;
	int i = cur_power_state;

	if (SYS_POWER_STATE_MAX == 0) {
		/* No power states to go through. */
		return SYS_POWER_STATE_ACTIVE;
	}

	do {
		i = (i + 1) % SYS_POWER_STATE_MAX;

#ifdef CONFIG_SYS_PM_STATE_LOCK
		if (!sys_pm_ctrl_is_state_enabled((enum power_states)(i))) {
			continue;
		}
#endif
		cur_power_state = i;

		LOG_DBG("Selected power state: %u", i);
		return (enum power_states)(i);
	} while (i != cur_power_state);

	LOG_DBG("No suitable power state found!");
	return SYS_POWER_STATE_ACTIVE;
}

__weak bool sys_pm_policy_low_power_devices(enum power_states pm_state)
{
	return sys_pm_is_sleep_state(pm_state);
}
