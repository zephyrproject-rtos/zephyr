/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <soc.h>

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		k_cpu_idle();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		PM->SLEEPCFG.bit.SLEEPMODE = 0x2;
		while (PM->SLEEPCFG.bit.SLEEPMODE != 0x2) {
		}
		__DSB();
		__ISB();
		__WFI();
		PM->SLEEPCFG.bit.SLEEPMODE = 0x0;
		break;
	default:
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}
