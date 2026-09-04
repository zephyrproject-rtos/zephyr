/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <cmsis_core.h>

#include "power_modes.h"

LOG_MODULE_REGISTER(soc_power, CONFIG_SOC_LOG_LEVEL);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE: {
		unsigned int key = arch_pm_state_set_prepare();

		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
		__WFI();

		arch_pm_state_set_finish(key);

		break;
	}

	case PM_STATE_SUSPEND_TO_IDLE:
		power_enter_deep_sleep();
		break;

#if defined(CONFIG_SOC_IMXRT7XX_POWER_DOMAIN_COMPUTE)
	case PM_STATE_STANDBY:
		power_enter_dsr();
		break;
#endif

	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	ARG_UNUSED(state);
}
