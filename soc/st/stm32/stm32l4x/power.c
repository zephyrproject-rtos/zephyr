/*
 * Copyright (c) 2019 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <zephyr/init.h>

#include <stm32l4xx_ll_utils.h>
#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_cortex.h>
#include <stm32l4xx_ll_pwr.h>
#include <stm32l4xx_ll_rcc.h>
#include <stm32l4xx_ll_system.h>
#include <clock_control/clock_stm32_ll_common.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* select MSI as wake-up system clock if configured, HSI otherwise */
#if STM32_SYSCLK_SRC_MSI
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_MSI
#else
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_HSI
#endif

void set_mode_stop(uint8_t substate_id)
{
	/* ensure the proper wake-up system clock */
	LL_RCC_SetClkAfterWakeFromStop(RCC_STOP_WAKEUPCLOCK_SELECTED);

	switch (substate_id) {
	case 1: /* this corresponds to the STOP0 mode: */
		/* enter STOP0 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0);
		break;
	case 2: /* this corresponds to the STOP1 mode: */
		/* enter STOP1 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);
		break;
	case 3: /* this corresponds to the STOP2 mode: */
#ifdef PWR_CR1_RRSTP
		LL_PWR_DisableSRAM3Retention();
#endif /* PWR_CR1_RRSTP */
		/* enter STOP2 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP2);
		break;
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		break;
	}
}

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		set_mode_stop(substate_id);
		/* Set SLEEPDEEP bit of Cortex System Control Register */
		LL_LPM_EnableDeepSleep();
		/* Select mode entry : WFE or WFI and enter the CPU selected mode */
		k_cpu_idle();
		break;
	case PM_STATE_STANDBY:
		LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
		LL_LPM_EnableDeepSleep();
		LL_DBGMCU_DisableDBGStandbyMode();
		k_cpu_idle();
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}

}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		if (substate_id <= 3) {
			LL_LPM_DisableSleepOnExit();
			LL_LPM_EnableSleep();
		} else {
			LOG_DBG("Unsupported power substate-id %u",
							substate_id);
		}
		/* need to restore the clock */
		stm32_clock_control_init(NULL);

		/*
		 * System is now in active mode.
		 * Reenable interrupts which were disabled
		 * when OS started idling code.
		 */
		irq_unlock(0);
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

}

/* Initialize STM32 Power */
static int stm32_power_init(void)
{

	/* enable Power clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	return 0;
}

SYS_INIT(stm32_power_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
