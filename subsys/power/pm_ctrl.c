/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <soc.h>
#include <device.h>
#include <atomic.h>
#include "policy/pm_policy.h"

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

struct pm_ctrl_info {
	int pm_state;
	atomic_t count;
};

static struct pm_ctrl_info pm_ctrl[] = {
	{SYS_PM_LOW_POWER_STATE, ATOMIC_INIT(0)},
	{SYS_PM_DEEP_SLEEP, ATOMIC_INIT(0)},
};

void sys_pm_ctrl_disable_state(int state)
{
	if (state == SYS_PM_LOW_POWER_STATE) {
		__ASSERT(pm_ctrl[0].count < UINT_MAX,
				"Low Power state count overflowed\n");
		atomic_inc(&pm_ctrl[0].count);
	} else if (state == SYS_PM_DEEP_SLEEP) {
		__ASSERT(pm_ctrl[1].count < UINT_MAX,
				"Deep Sleep state count overflowed\n");
		atomic_inc(&pm_ctrl[1].count);
	} else {
		LOG_WRN("\nInvalid PM state");
	}
}

void sys_pm_ctrl_enable_state(int state)
{
	if (state == SYS_PM_LOW_POWER_STATE) {
		__ASSERT(pm_ctrl[0].count > 0,
				"Low Power state count underflowed\n");
		atomic_dec(&pm_ctrl[0].count);
	} else if (state == SYS_PM_DEEP_SLEEP) {
		__ASSERT(pm_ctrl[1].count > 0,
				"Deep Sleep state count underflowed\n");
		atomic_dec(&pm_ctrl[1].count);
	} else {
		LOG_WRN("\nInvalid PM state");
	}
}

bool sys_pm_ctrl_is_state_enabled(int state)
{
	bool enabled = true;

	switch (state) {
	case SYS_PM_LOW_POWER_STATE:
		if (pm_ctrl[0].count) {
			enabled = false;
		}
		break;
	case SYS_PM_DEEP_SLEEP:
		if (pm_ctrl[1].count) {
			enabled = false;
		}
		break;
	default:
		LOG_WRN("\nInvalid PM state");
		enabled = false;
	}

	return enabled;
}
