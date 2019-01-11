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

#if !(defined(CONFIG_SYS_POWER_STATE_CPU_LPS_SUPPORTED) || \
		defined(CONFIG_SYS_POWER_STATE_CPU_LPS_1_SUPPORTED) || \
		defined(CONFIG_SYS_POWER_STATE_CPU_LPS_2_SUPPORTED) || \
		defined(CONFIG_SYS_POWER_STATE_DEEP_SLEEP_SUPPORTED) || \
		defined(CONFIG_SYS_POWER_STATE_DEEP_SLEEP_1_SUPPORTED) || \
		defined(CONFIG_SYS_POWER_STATE_DEEP_SLEEP_2_SUPPORTED))
#error "Enable Low Power States at SoC Level"
#endif

struct sys_pm_policy {
	enum power_states pm_state;
	int sys_state;
};

/* PM Policy based on SoC/Platform residency requirements */
static struct sys_pm_policy pm_policy[] = {
#ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_SUPPORTED
	{SYS_POWER_STATE_CPU_LPS, SYS_PM_LOW_POWER_STATE},
#endif

#ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_1_SUPPORTED
	{SYS_POWER_STATE_CPU_LPS_1, SYS_PM_LOW_POWER_STATE},
#endif

#ifdef CONFIG_SYS_POWER_STATE_CPU_LPS_2_SUPPORTED
	{SYS_POWER_STATE_CPU_LPS_2, SYS_PM_LOW_POWER_STATE},
#endif

#ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_SUPPORTED
	{SYS_POWER_STATE_DEEP_SLEEP, SYS_PM_DEEP_SLEEP},
#endif

#ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_1_SUPPORTED
	{SYS_POWER_STATE_DEEP_SLEEP_1, SYS_PM_DEEP_SLEEP},
#endif

#ifdef CONFIG_SYS_POWER_STATE_DEEP_SLEEP_2_SUPPORTED
	{SYS_POWER_STATE_DEEP_SLEEP_2, SYS_PM_DEEP_SLEEP},
#endif
};

int sys_pm_policy_next_state(s32_t ticks, enum power_states *pm_state)
{
	static int cur_pm_idx;
	int i = cur_pm_idx;

	do {
		i = (i + 1) % ARRAY_SIZE(pm_policy);

#ifdef CONFIG_PM_CONTROL_STATE_LOCK
		if (!sys_pm_ctrl_is_state_enabled(pm_policy[i].pm_state)) {
			continue;
		}
#endif
		cur_pm_state = i;
		*pm_state = pm_policy[cur_pm_state].pm_state;

		LOG_DBG("pm_state: %d, idx: %d\n", *pm_state, i);
		return pm_policy[cur_pm_state].sys_state;
	} while (i != curr_pm_idx);

	LOG_DBG("No suitable power state found!");
	return SYS_PM_NOT_HANDLED;
}
