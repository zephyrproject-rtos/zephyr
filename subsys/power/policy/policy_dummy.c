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

struct sys_soc_pm_policy {
	enum power_states pm_state;
	int sys_state;
};

/* PM Policy based on SoC/Platform residency requirements */
static struct sys_soc_pm_policy pm_policy[] = {
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

	if (cur_pm_idx >= ARRAY_SIZE(pm_policy)) {
		cur_pm_idx = 0;
	}

	if (!sys_soc_is_valid_power_state(pm_policy[cur_pm_idx].pm_state)) {
		LOG_ERR("pm_state(%d) not supported by SoC\n",
						pm_policy[cur_pm_idx].pm_state);
		return SYS_PM_NOT_HANDLED;
	}

	*pm_state = pm_policy[cur_pm_idx].pm_state;
	LOG_DBG("pm_state: %d, idx: %d\n", *pm_state, cur_pm_idx);

	return pm_policy[cur_pm_idx++].sys_state;
}
