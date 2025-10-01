/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>

void pm_state_set(enum pm_state state, uint8_t id)
{
	ARG_UNUSED(id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		k_cpu_idle();
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
