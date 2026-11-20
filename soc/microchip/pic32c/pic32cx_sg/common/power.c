/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arch_interface.h>
#include <zephyr/arch/arm/misc.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>

#include <soc.h>

static void enter_idle(void)
{
	unsigned int key;

	/* The write reaches PM through a bridge: wait for it before WFI. */
	PM_REGS->PM_SLEEPCFG = PM_SLEEPCFG_SLEEPMODE_IDLE;
	while (PM_REGS->PM_SLEEPCFG != PM_SLEEPCFG_SLEEPMODE_IDLE) {
	}

	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

	key = arch_pm_state_set_prepare();
	__WFI();
	arch_pm_state_set_finish(key);
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		enter_idle();
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

/*
 * The idle sleep stops the AHB clock, which bus master peripherals such as the M_CAN need. The
 * CPU only sleeps through pm_state_set(), so that a driver can keep it awake with a power state
 * lock: when the PM policy selects no state, the idle thread does not execute WFI.
 */
bool z_arm_on_enter_cpu_idle(void)
{
	return false;
}
