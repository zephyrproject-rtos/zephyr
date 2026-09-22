/*
 * Copyright (c) 2025 James Bennion-Pedley
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/pm/pm.h>

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		break;
	default:
		return;
	}

	__asm__ volatile("wfi");
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	irq_unlock(MSTATUS_IEN);
}
