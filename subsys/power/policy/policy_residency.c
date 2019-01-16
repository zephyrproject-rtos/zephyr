/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <soc.h>
#include "pm_policy.h"

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

#define SECS_TO_TICKS		CONFIG_SYS_CLOCK_TICKS_PER_SEC

/* PM Policy based on SoC/Platform residency requirements */
static const unsigned int pm_min_residency[] = {
#ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_SUPPORTED
	CONFIG_PM_LPS_MIN_RES * SECS_TO_TICKS,
#endif

#ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_1_SUPPORTED
	CONFIG_PM_LPS_1_MIN_RES * SECS_TO_TICKS,
#endif

#ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_2_SUPPORTED
	CONFIG_PM_LPS_2_MIN_RES * SECS_TO_TICKS,
#endif

#ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_SUPPORTED
	CONFIG_PM_DEEP_SLEEP_MIN_RES * SECS_TO_TICKS,
#endif

#ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_1_SUPPORTED
	CONFIG_PM_DEEP_SLEEP_1_MIN_RES * SECS_TO_TICKS,
#endif

#ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_2_SUPPORTED
	CONFIG_PM_DEEP_SLEEP_2_MIN_RES * SECS_TO_TICKS,
#endif
};

enum power_states sys_pm_policy_next_state(s32_t ticks)
{
	int i;

	if ((ticks != K_FOREVER) && (ticks < pm_min_residency[0])) {
		LOG_ERR("Not enough time for PM operations: %d", ticks);
		return SYS_POWER_STATE_ACTIVE;
	}

	for (i = ARRAY_SIZE(pm_min_residency) - 1; i >= 0; i--) {
#ifdef CONFIG_PM_CONTROL_STATE_LOCK
		if (!sys_pm_ctrl_is_state_enabled((enum power_states)(i))) {
			continue;
		}
#endif
		if ((ticks == K_FOREVER) ||
		    (ticks >= pm_min_residency[i])) {
			LOG_DBG("Selected power state %d "
					"(ticks: %d, min_residency: %u)",
					i, ticks, pm_min_residency[i]);
			return (enum power_states)(i);
		}
	}

	LOG_DBG("No suitable power state found!");
	return SYS_POWER_STATE_ACTIVE;
}
