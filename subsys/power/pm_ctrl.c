/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <device.h>
#include <sys/atomic.h>
#include "policy/pm_policy.h"

#define LOG_LEVEL CONFIG_SYS_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

static atomic_t power_state_enable_count[SYS_POWER_STATE_MAX];

void sys_pm_ctrl_disable_state(enum power_states state)
{
	atomic_val_t v;

	__ASSERT(state < SYS_POWER_STATE_MAX, "Invalid power state!");
	__ASSERT_NO_MSG(state >= 0);
	v = atomic_dec(&power_state_enable_count[state]);
	__ASSERT(v > INT_MIN, "Power state enable count underflowed!");

	/* Make compiler happy when assertions are disabled. */
	(void)(v);
}

void sys_pm_ctrl_enable_state(enum power_states state)
{
	atomic_val_t v;

	__ASSERT(state < SYS_POWER_STATE_MAX, "Invalid power state!");
	__ASSERT_NO_MSG(state >= 0);
	v = atomic_inc(&power_state_enable_count[state]);
	__ASSERT(v < 1, "Power state enable count overflowed!");

	/* Make compiler happy when assertions are disabled. */
	(void)(v);
}

bool sys_pm_ctrl_is_state_enabled(enum power_states state)
{
	__ASSERT(state < SYS_POWER_STATE_MAX, "Invalid power state!");
	__ASSERT_NO_MSG(state >= 0);

	return (atomic_get(&power_state_enable_count[state]) > 0);
}
