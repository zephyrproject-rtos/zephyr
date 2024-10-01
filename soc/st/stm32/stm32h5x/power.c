/*
 * Copyright (c) 2024 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <zephyr/init.h>

#include <stm32h5xx_ll_cortex.h>
#include <stm32h5xx_ll_pwr.h>
#include <clock_control/clock_stm32_ll_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	if (state != PM_STATE_SUSPEND_TO_IDLE) {
		LOG_DBG("Unsupported power state %u", state);
		return;
	}

	switch (substate_id) {
	case 1: /* this corresponds to the STOP mode: */
		/* enter STOP mode */
		LL_PWR_SetPowerMode(LL_PWR_STOP_MODE);
		LL_LPM_EnableDeepSleep();
		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();
		break;
	default:
		LOG_DBG("Unsupported power state substate-id %u",
			substate_id);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	if (state != PM_STATE_SUSPEND_TO_IDLE) {
		LOG_DBG("Unsupported power substate %u", state);
	} else {
		switch (substate_id) {
		case 1:	/* STOP */
			LL_LPM_DisableSleepOnExit();
			/* Clear SLEEPDEEP bit */
			LL_LPM_EnableSleep();
			break;
		default:
			LOG_DBG("Unsupported power substate-id %u",
				substate_id);
			break;
		}
		/* need to restore the clock */
		stm32_clock_control_init(NULL);
	}

	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}

/* Initialize STM32 Power */
void stm32_power_init(void)
{
}
