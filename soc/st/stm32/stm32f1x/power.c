/*
 * Copyright (c) 2025 Oleh Kravchenko <oleh@kaa.org.ua>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <clock_control/clock_stm32_ll_common.h>
#include <stm32f1xx_ll_cortex.h>
#include <stm32f1xx_ll_pwr.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		LL_LPM_DisableEventOnPend();
		LL_PWR_ClearFlag_WU();

		if (substate_id == 0) {
			/* 250 uA */
			LL_PWR_SetPowerMode(LL_PWR_MODE_STOP_MAINREGU);
		} else {
			/* 245 uA */
			LL_PWR_SetPowerMode(LL_PWR_MODE_STOP_LPREGU);
		}

		LL_LPM_EnableDeepSleep();
		k_cpu_idle();
		break;

	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		LL_LPM_DisableSleepOnExit();
		LL_LPM_EnableSleep();

		/* Restore the clock setup. */
		stm32_clock_control_init(NULL);
		break;

	default:
		LOG_DBG("Unsupported power substate-id %u", state);
		break;
	}

	/*
	 * System is now in active mode. Reenable interrupts which were
	 * disabled when OS started idling code.
	 */
	irq_unlock(0);
}
