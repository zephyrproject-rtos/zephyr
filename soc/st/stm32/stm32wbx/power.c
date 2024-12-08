/*
 * Copyright (c) 2019 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <stm32wbxx_ll_utils.h>
#include <stm32wbxx_ll_bus.h>
#include <stm32wbxx_ll_cortex.h>
#include <stm32wbxx_ll_pwr.h>
#include <stm32wbxx_ll_rcc.h>
#include <clock_control/clock_stm32_ll_common.h>
#include "stm32_hsem.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/*
 * @brief Switch the system clock on HSI
 * @param none
 * @retval none
 */
static void switch_on_hsi(void)
{
	LL_RCC_HSI_Enable();
	while (!LL_RCC_HSI_IsReady()) {
	}

	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
	LL_RCC_SetSMPSClockSource(LL_RCC_SMPS_CLKSOURCE_HSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
	}
}

static void lpm_hsem_lock(void)
{
	/* Implementation of STM32 AN5289 algorithm to enter/exit lowpower */
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_WAIT_FOREVER);

	if (!LL_HSEM_1StepLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID)) {
		if (LL_PWR_IsActiveFlag_C2DS()) {
			/* Release ENTRY_STOP_MODE semaphore */
			LL_HSEM_ReleaseLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID, 0);

			/* The switch on HSI before entering Stop Mode is required */
			switch_on_hsi();
		}
	} else {
		/* The switch on HSI before entering Stop Mode is required */
		switch_on_hsi();
	}
}

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	if (state == PM_STATE_SUSPEND_TO_IDLE) {

		lpm_hsem_lock();

		/* ensure HSI is the wake-up system clock */
		LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);

		switch (substate_id) {
		case 1:
			/* enter STOP0 mode */
			LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0);
			break;
		case 2:
			/* enter STOP1 mode */
			LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);
			break;
		case 3:
			/* enter STOP2 mode */
			LL_PWR_SetPowerMode(LL_PWR_MODE_STOP2);
			break;
		default:
			/* Release RCC semaphore */
			z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);
			LOG_DBG("Unsupported power substate-id %u", substate_id);
			return;
		}

		if (IS_ENABLED(STM32_HSI48_ENABLED)) {
			/*
			 * Release CLK48 semaphore to make sure M0 core can enable/disable
			 * it as needed (shared between RNG and USB peripheral, M0 uses RNG
			 * during BLE advertisement phase). It seems like if left locked M0
			 * can enable the clock if needed but is not able (allowed) to stop
			 * it, with increased power consumption as a result.
			 */
			z_stm32_hsem_unlock(CFG_HW_CLK48_CONFIG_SEMID);
		}

		/* Release RCC semaphore */
		z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

		LL_LPM_EnableDeepSleep();

		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();
	} else {
		LOG_DBG("Unsupported power state %u", state);
		return;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	/* Implementation of STM32 AN5289 algorithm to enter/exit lowpower */
	/* Release ENTRY_STOP_MODE semaphore */
	LL_HSEM_ReleaseLock(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID, 0);
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_WAIT_FOREVER);

	if (state != PM_STATE_SUSPEND_TO_IDLE) {
		LOG_DBG("Unsupported power state %u", state);
	} else {
		switch (substate_id) {
		case 1:	/* STOP0 */
			__fallthrough;
		case 2:	/* STOP1 */
			__fallthrough;
		case 3:	/* STOP2 */
			LL_LPM_DisableSleepOnExit();
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

	/* Release RCC semaphore */
	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}

/* Initialize STM32 Power */
void stm32_pm_init(void)
{
}
