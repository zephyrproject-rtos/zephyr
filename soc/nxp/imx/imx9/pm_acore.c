/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/pm_cpu_ops/psci.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	disable_irq();

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		psci_cpu_suspend(state, 0);
		break;
	default:
		LOG_ERR("PM state %d is not supported.", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	enable_irq();
}
