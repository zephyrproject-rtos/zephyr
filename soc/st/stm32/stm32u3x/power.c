/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <zephyr/init.h>

#include <stm32_ll_utils.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_cortex.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include <clock_control/clock_stm32_ll_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* select MSI as wake-up system clock if configured, HSI otherwise */
#if STM32_SYSCLK_SRC_MSIS
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUP_CLK_MSIS
#else
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUP_CLK_HSI16
#endif

void set_mode_stop(uint8_t substate_id)
{
	/* ensure the proper wake-up system clock */
	LL_RCC_SetStopWakeupClock(RCC_STOP_WAKEUPCLOCK_SELECTED);

	switch (substate_id) {
	case 1: /* enter STOP0 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0);
		break;
	case 2: /* enter STOP1 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);
		break;
	case 3: /* enter STOP2 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP2);
		break;
#ifdef CONFIG_STM32_STOP3_LP_MODE
	case 4: /* enter STOP3 mode */

		LL_PWR_SetSRAM2SBRetention(LL_PWR_SRAM2_SB_FULL_RETENTION);
		/* Enable RTC wakeup
		 * This configures an internal pin that generates an event to wakeup the system
		 */
		LL_PWR_EnableWakeUpLine(LL_PWR_WAKEUP_LINE7);
		LL_PWR_SetWakeUpLineSignal3Selection(LL_PWR_WAKEUP_LINE7);

		/* Clear flags */
		LL_PWR_ClearFlag_SB();
		LL_PWR_ClearFlag_WU();

		sys_cache_instr_disable();

		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP3);
		break;
#endif
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		break;
	}
}

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	uint32_t key;

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		set_mode_stop(substate_id);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}

	/* Set SLEEPDEEP bit of Cortex System Control Register */
	LL_LPM_EnableDeepSleep();

	/* Select mode entry : WFE or WFI and enter the CPU selected mode */
	key = arch_pm_state_set_prepare();
	__WFI();
	arch_pm_state_set_finish(key);

	LL_LPM_DisableSleepOnExit();
	LL_LPM_EnableSleep();
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		if (substate_id <= 3) {
#ifdef CONFIG_STM32_STOP3_LP_MODE
		} else if (substate_id == 4) {
			stm32_clock_control_standby_exit();

			sys_cache_instr_enable();
#endif
		} else {
			LOG_DBG("Unsupported power substate-id %u", substate_id);
		}
		break;
	case PM_STATE_STANDBY:
		__fallthrough;
	case PM_STATE_SUSPEND_TO_RAM:
		__fallthrough;
	case PM_STATE_SUSPEND_TO_DISK:
		__fallthrough;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
	/* need to restore the clock */
	stm32_clock_control_init(NULL);
}
