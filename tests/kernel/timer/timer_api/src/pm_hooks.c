/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#ifdef CONFIG_PM

#include <zephyr/pm/pm.h>

/*
 * CONFIG_PM needs an SoC to implement these two hooks, and only SoCs with real
 * low-power states do. This test wants the PM idle path, not a low-power mode,
 * so pm_state_set() just waits for the next interrupt the way an ordinary idle
 * would. That is enough to reach sys_clock_idle_enter() and, on the way out,
 * sys_clock_idle_exit().
 *
 * k_cpu_idle() returns with interrupts unlocked, hence
 * CONFIG_PM_STATE_SET_IRQ_UNLOCKED in the test configuration.
 */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	k_cpu_idle();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}

#endif /* CONFIG_PM */
