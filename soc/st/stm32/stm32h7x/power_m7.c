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
#include <stm32_ll_rcc.h>
#include <clock_control/clock_stm32_ll_common.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

static void switch_sysclk_to_hsi(void)
{
	/* Enable HSI if not enabled */
	if (LL_RCC_HSI_IsReady() != 1) {
		/* Enable HSI */
		LL_RCC_HSI_Enable();
		while (LL_RCC_HSI_IsReady() != 1) {
			/* Wait for HSI ready */
		}
	}

	/* Set HSI as SYSCLCK source */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
	}
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	if (state != PM_STATE_SUSPEND_TO_IDLE) {
		LOG_DBG("Unsupported power state %u", state);
		return;
	}

	/* Clear previous STOP/Standby flags */
	LL_PWR_ClearFlag_CPU();

#if defined(SYSCFG_PWRCR_ODEN)
	/*
	 * RM0433 Rev 8 §6.6.2 (Note, p. 280):
	 * "VOS0 deactivation must be managed by software before the system
	 * enters low-power mode"; §6.7.8 (p. 297) says the same for Stop.
	 * §6.6.2 gives the sequence: lower the system frequency, then clear
	 * ODEN in SYSCFG_PWRCR with the SYSCFG clock enabled.
	 */
	LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_SYSCFG);
	if (SYSCFG->PWRCR & SYSCFG_PWRCR_ODEN) {
		/* Lower the system frequency. */
		switch_sysclk_to_hsi();

		/* Disable PLL1, PLL2 and PLL3 as they can still be feeding
		 * kernel clocks, and several of those have lower limits at VOS1
		 * than at VOS0. Stop turns them off anyway, so just do it now
		 * to avoid overclocking after ODEN bit is cleared.
		 */
		LL_RCC_PLL1_Disable();
		LL_RCC_PLL2_Disable();
		LL_RCC_PLL3_Disable();

		/* Clear ODEN and transition to VOS1. */
		SYSCFG->PWRCR &= ~SYSCFG_PWRCR_ODEN;
	}
#endif

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
}

void stm32_power_init(void)
{
	/* Nothing to do. */
}

#endif
