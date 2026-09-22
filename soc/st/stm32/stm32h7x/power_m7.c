/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_PM)

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <zephyr/init.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_cortex.h>
#include <stm32_ll_pwr.h>
#include <clock_control/clock_stm32_ll_common.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	if (state != PM_STATE_SUSPEND_TO_IDLE) {
		LOG_DBG("Unsupported power state %u", state);
		return;
	}

	/* Clear previous STOP/Standby flags */
	LL_PWR_ClearFlag_CPU();

	/* Configure SVOS5 and Flash power-down in STOP mode */
	LL_PWR_SetStopModeRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SVOS_SCALE5);
	LL_PWR_EnableFlashPowerDown();

	/* Enter D1/D2/D3 STOP mode when CPU enters CStop */
	LL_PWR_CPU_SetD1PowerMode(LL_PWR_CPU_MODE_D1STOP);
	LL_PWR_CPU_SetD2PowerMode(LL_PWR_CPU_MODE_D2STOP);
	LL_PWR_CPU_SetD3PowerMode(LL_PWR_CPU_MODE_D3STOP);

	LL_LPM_EnableDeepSleep();

	k_cpu_idle();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	if (state == PM_STATE_SUSPEND_TO_IDLE) {
		LL_LPM_DisableSleepOnExit();
		LL_LPM_EnableSleep();
		LL_PWR_ClearFlag_CPU();

		/* Restore clock tree configured in Device Tree */
		stm32_clock_control_init(NULL);
	}

	irq_unlock(0);
}

void stm32_power_init(void)
{
	/* Nothing to do. */
}

#endif
