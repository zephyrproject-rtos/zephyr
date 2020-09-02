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

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

static atomic_t power_state_disable_count[POWER_STATE_MAX];

void pm_ctrl_disable_state(enum power_states state)
{
	atomic_val_t v;

	__ASSERT(state < POWER_STATE_MAX, "Invalid power state!");
	v = atomic_inc(&power_state_disable_count[state]);
	__ASSERT(v < UINT_MAX, "Power state disable count overflowed!");

	/* Make compiler happy when assertions are disabled. */
	(void)(v);
}

void pm_ctrl_enable_state(enum power_states state)
{
	atomic_val_t v;

	__ASSERT(state < POWER_STATE_MAX, "Invalid power state!");
	v = atomic_dec(&power_state_disable_count[state]);
	__ASSERT(v > 0, "Power state disable count underflowed!");

	/* Make compiler happy when assertions are disabled. */
	(void)(v);
}

bool pm_ctrl_is_state_enabled(enum power_states state)
{
	__ASSERT(state < POWER_STATE_MAX, "Invalid power state!");

	return (atomic_get(&power_state_disable_count[state]) == 0);
}
