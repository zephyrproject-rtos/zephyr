/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/atomic.h>
#include <pm/state.h>
#include <toolchain.h>

static atomic_t power_state_disable_count[PM_STATE_COUNT];

void pm_constraint_set(enum pm_state state)
{
	atomic_inc(&power_state_disable_count[state]);
}

void pm_constraint_release(enum pm_state state)
{
	atomic_dec(&power_state_disable_count[state]);
}

bool pm_constraint_get(enum pm_state state)
{
	return (atomic_get(&power_state_disable_count[state]) == 0);
}
