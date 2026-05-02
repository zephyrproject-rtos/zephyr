/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>

LOG_MODULE_REGISTER(soc_pm, CONFIG_APP_LOG_LEVEL);

static const char *state2str(enum pm_state state)
{
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		return "RUNTIME_IDLE";
	case PM_STATE_SUSPEND_TO_IDLE:
		return "SUSPEND_TO_IDLE";
	case PM_STATE_STANDBY:
		return "STANDBY";
	case PM_STATE_SUSPEND_TO_RAM:
		return "SUSPEND_TO_RAM";
	case PM_STATE_SUSPEND_TO_DISK:
		return "SUSPEND_TO_DISK";
	case PM_STATE_SOFT_OFF:
		return "SOFT_OFF";
	default:
		return "UNKNOWN";
	}
}

/*
 * PM callbacks are typically implemented at SoC level and run the actual
 * code to enter the given PM state.
 */

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	LOG_INF("Entering %s{%u}", state2str(state), substate_id);
	k_cpu_idle();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}
