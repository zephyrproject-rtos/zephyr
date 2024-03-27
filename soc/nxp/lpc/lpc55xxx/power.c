/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/pm.h>

#include "fsl_power.h"

void pm_state_set(enum pm_state state, uint8_t id)
{
	ARG_UNUSED(id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		POWER_EnterSleep();
		break;
	default:
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(id);
}
