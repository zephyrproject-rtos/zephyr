/*
 * Copyright (c) 2026 Gabriel Germano
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h>

#include <cmsis_core.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
	case PM_STATE_SUSPEND_TO_IDLE:
		__disable_irq();
		__WFI();
		break;
	default:
		LOG_DBG("PM state not supported: %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
	__enable_irq();
}
