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

#ifdef CONFIG_TICKLESS_KERNEL
#define SECS_TO_TICKS		CONFIG_TICKLESS_KERNEL_TIME_UNIT_IN_MICRO_SECS
#else
#define SECS_TO_TICKS		CONFIG_SYS_CLOCK_TICKS_PER_SEC
#endif

#if !(defined(CONFIG_PM_CONTROL_OS_LPS) || \
		defined(CONFIG_PM_CONTROL_OS_LPS_1) || \
		defined(CONFIG_PM_CONTROL_OS_LPS_2) || \
		defined(CONFIG_PM_CONTROL_OS_DEEP_SLEEP) || \
		defined(CONFIG_PM_CONTROL_OS_DEEP_SLEEP_1) || \
		defined(CONFIG_PM_CONTROL_OS_DEEP_SLEEP_2))
#error "Low Power states not selected by policy"
#endif

struct sys_soc_pm_policy {
	enum power_states pm_state;
	int sys_state;
	int min_residency;
};

/* PM Policy based on SoC/Platform residency requirements */
static struct sys_soc_pm_policy pm_policy[] = {
#ifdef CONFIG_PM_CONTROL_OS_LPS
	{SYS_POWER_STATE_CPU_LPS, SYS_PM_LOW_POWER_STATE,
			CONFIG_PM_LPS_MIN_RES * SECS_TO_TICKS},
#endif

#ifdef CONFIG_PM_CONTROL_OS_LPS_1
	{SYS_POWER_STATE_CPU_LPS_1, SYS_PM_LOW_POWER_STATE,
			CONFIG_PM_LPS_1_MIN_RES * SECS_TO_TICKS},
#endif

#ifdef CONFIG_PM_CONTROL_OS_LPS_2
	{SYS_POWER_STATE_CPU_LPS_2, SYS_PM_LOW_POWER_STATE,
			CONFIG_PM_LPS_2_MIN_RES * SECS_TO_TICKS},
#endif

#ifdef CONFIG_PM_CONTROL_OS_DEEP_SLEEP
	{SYS_POWER_STATE_DEEP_SLEEP, SYS_PM_DEEP_SLEEP,
			CONFIG_PM_DEEP_SLEEP_MIN_RES * SECS_TO_TICKS},
#endif

#ifdef CONFIG_PM_CONTROL_OS_DEEP_SLEEP_1
	{SYS_POWER_STATE_DEEP_SLEEP_1, SYS_PM_DEEP_SLEEP,
			CONFIG_PM_DEEP_SLEEP_1_MIN_RES * SECS_TO_TICKS},
#endif

#ifdef CONFIG_PM_CONTROL_OS_DEEP_SLEEP_2
	{SYS_POWER_STATE_DEEP_SLEEP_2, SYS_PM_DEEP_SLEEP,
			CONFIG_PM_DEEP_SLEEP_2_MIN_RES * SECS_TO_TICKS},
#endif
};

int sys_pm_policy_next_state(s32_t ticks, enum power_states *pm_state)
{
	int i;

	if ((ticks != K_FOREVER) && (ticks < pm_policy[0].min_residency)) {
		LOG_ERR("Not enough time for PM operations: %d\n", ticks);
		return SYS_PM_NOT_HANDLED;
	}

	for (i = 0; i < (ARRAY_SIZE(pm_policy) - 1); i++) {
		if ((ticks >= pm_policy[i].min_residency) &&
			       (ticks < pm_policy[i + 1].min_residency)) {
			break;
		}
	}

	if (!_sys_soc_is_valid_power_state(pm_policy[i].pm_state)) {
		LOG_ERR("pm_state(%d) not supported by SoC\n",
						pm_policy[i].pm_state);
		return SYS_PM_NOT_HANDLED;
	}

	*pm_state = pm_policy[i].pm_state;
	LOG_DBG("pm_state: %d, min_residency: %d, idx: %d\n",
				*pm_state, pm_policy[i].min_residency, i);

	return pm_policy[i].sys_state;
}
