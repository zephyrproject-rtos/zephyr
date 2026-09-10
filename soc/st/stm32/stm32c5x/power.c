/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <clock_control/clock_stm32_ll_common.h>
#include <soc.h>

#include <stm32_ll_pwr.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* select HSIS as wake-up system clock if configured, HSIDIV3 otherwise */
#if STM32_SYSCLK_SRC_HSIS
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_HSIS
#else
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_HSIDIV3
#endif

void set_mode_stop(uint8_t substate_id)
{
	/* ensure the proper wake-up system clock */
	LL_RCC_SetClkAfterWakeFromStop(RCC_STOP_WAKEUPCLOCK_SELECTED);

	switch (substate_id) {
	case 1: /* enter STOP0 mode */
		LL_PWR_SetPowerMode(LL_PWR_STOP0_MODE);
		break;
	case 2: /* enter STOP1 mode */
		LL_PWR_SetPowerMode(LL_PWR_STOP1_MODE);
		break;
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		break;
	}
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	unsigned int key;

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		set_mode_stop(substate_id);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}

	/* Set SLEEPDEEP bit of Cortex System Control Register */
	SCB_EnableDeepSleep();

	/* Enter low-power mode */
	key = arch_pm_state_set_prepare();
	__WFI();
	arch_pm_state_set_finish(key);

	/* Reset SLEEPDEEP bit of Cortex System Control Register */
	SCB_DisableDeepSleep();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		/* Restore the clock setup. */
		stm32_clock_control_init(NULL);
		break;
	default:
		LOG_WRN("Unsupported power substate-id %u", state);
		break;
	}
}
