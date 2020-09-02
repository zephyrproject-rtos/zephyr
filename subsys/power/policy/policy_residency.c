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

#define SECS_TO_TICKS		CONFIG_SYS_CLOCK_TICKS_PER_SEC

/* PM Policy based on SoC/Platform residency requirements */
static const unsigned int pm_min_residency[] = {
#ifdef CONFIG_PM_SLEEP_STATES
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_1
	CONFIG_PM_MIN_RESIDENCY_SLEEP_1 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif

#ifdef CONFIG_HAS_POWER_STATE_SLEEP_2
	CONFIG_PM_MIN_RESIDENCY_SLEEP_2 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif

#ifdef CONFIG_HAS_POWER_STATE_SLEEP_3
	CONFIG_PM_MIN_RESIDENCY_SLEEP_3 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif
#endif /* CONFIG_PM_SLEEP_STATES */

#ifdef CONFIG_PM_DEEP_SLEEP_STATES
#ifdef CONFIG_HAS_POWER_STATE_DEEP_SLEEP_1
	CONFIG_PM_MIN_RESIDENCY_DEEP_SLEEP_1 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif

#ifdef CONFIG_HAS_POWER_STATE_DEEP_SLEEP_2
	CONFIG_PM_MIN_RESIDENCY_DEEP_SLEEP_2 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif

#ifdef CONFIG_HAS_POWER_STATE_DEEP_SLEEP_3
	CONFIG_PM_MIN_RESIDENCY_DEEP_SLEEP_3 * SECS_TO_TICKS / MSEC_PER_SEC,
#endif
#endif /* CONFIG_PM_DEEP_SLEEP_STATES */
};

enum power_states pm_policy_next_state(int32_t ticks)
{
	int i;

	if ((ticks != K_TICKS_FOREVER) && (ticks < pm_min_residency[0])) {
		LOG_DBG("Not enough time for PM operations: %d", ticks);
		return POWER_STATE_ACTIVE;
	}

	for (i = ARRAY_SIZE(pm_min_residency) - 1; i >= 0; i--) {
#ifdef CONFIG_PM_STATE_LOCK
		if (!pm_ctrl_is_state_enabled((enum power_states)(i))) {
			continue;
		}
#endif
		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= pm_min_residency[i])) {
			LOG_DBG("Selected power state %d "
					"(ticks: %d, min_residency: %u)",
					i, ticks, pm_min_residency[i]);
			return (enum power_states)(i);
		}
	}

	LOG_DBG("No suitable power state found!");
	return POWER_STATE_ACTIVE;
}

__weak bool pm_policy_low_power_devices(enum power_states pm_state)
{
	return pm_is_sleep_state(pm_state);
}
