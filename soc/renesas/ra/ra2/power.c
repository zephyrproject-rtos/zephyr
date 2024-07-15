/**
 * power.c
 *
 * RA2 power management implementation
 *
 * Copyright (c) 2022-2024 MUNIC Car Data
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/lpm/lpm_ra2.h>

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_RAM:
		lpm_enter_standby();
		/* fallthrough */
	case PM_STATE_RUNTIME_IDLE:
		k_cpu_idle();
		break;
	default:
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_RAM:
		lpm_leave_standby();
		break;
	default:
		break;
	}

	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}
