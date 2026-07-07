/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>

#include "power_modes.h"

LOG_MODULE_REGISTER(soc_power, CONFIG_SOC_LOG_LEVEL);

#define DEEP_SLEEP_CONFIG									\
	((const uint32_t[])DT_PROP_OR(DT_NODELABEL(deepsleep), deep_sleep_config, {}))

#if defined(CONFIG_SOC_IMXRT7XX_POWER_DOMAIN_COMPUTE)
#define DEEP_SLEEP_RETENTION_CONFIG								\
	((const uint32_t[])DT_PROP_OR(DT_NODELABEL(dsr), deep_sleep_retention_config, {}))
#endif

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
		power_enter_deep_sleep(DEEP_SLEEP_CONFIG);
		break;

#if defined(CONFIG_SOC_IMXRT7XX_POWER_DOMAIN_COMPUTE)
	case PM_STATE_STANDBY:
		/* The OS Timer is reset by DSR and cannot wake the system; the
		 * system-timer low-power companion (armed by the timer driver on
		 * idle entry, see CONFIG_SYSTEM_TIMER_RESET_BY_LPM) owns wakeup.
		 */
		power_enter_dsr(DEEP_SLEEP_RETENTION_CONFIG);
		break;
#endif

	default:
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	ARG_UNUSED(state);
}
