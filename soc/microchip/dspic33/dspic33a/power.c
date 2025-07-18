/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <xc.h>

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		__builtin_disable_interrupts();
		Idle();
		break;
	default:
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		__builtin_enable_interrupts();
		break;
	default:
		break;
	}
}
