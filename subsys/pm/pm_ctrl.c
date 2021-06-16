/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <device.h>
#include <sys/atomic.h>
#include <pm/state.h>

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

#define PM_STATES_LEN (1 + PM_STATE_SOFT_OFF - PM_STATE_ACTIVE)

static atomic_t power_state_disable_count[PM_STATES_LEN];

void pm_constraint_set(enum pm_state state)
{
	atomic_val_t v;

	__ASSERT(state < PM_STATES_LEN, "Invalid power state!");
	v = atomic_inc(&power_state_disable_count[state]);
	__ASSERT(v < UINT_MAX, "Power state disable count overflowed!");

	/* Make compiler happy when assertions are disabled. */
	(void)(v);
}

void pm_constraint_release(enum pm_state state)
{
	atomic_val_t v;

	__ASSERT(state < PM_STATES_LEN, "Invalid power state!");
	v = atomic_dec(&power_state_disable_count[state]);
	__ASSERT(v > 0, "Power state disable count underflowed!");

	/* Make compiler happy when assertions are disabled. */
	(void)(v);
}

bool pm_constraint_get(enum pm_state state)
{
	__ASSERT(state < PM_STATES_LEN, "Invalid power state!");

	return (atomic_get(&power_state_disable_count[state]) == 0);
}
