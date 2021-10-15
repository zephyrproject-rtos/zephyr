/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <pm/pm.h>
#include <pm/policy.h>

__weak struct pm_state_info pm_policy_next_state(int32_t ticks)
{
	/*
	 * TODO: implement deep doze mode
	 * If the clock of any peripheral modules are not enabled,
	 * it is allowed to enter deep doze.
	 */

	return (struct pm_state_info){PM_STATE_ACTIVE, 0, 0};
}
