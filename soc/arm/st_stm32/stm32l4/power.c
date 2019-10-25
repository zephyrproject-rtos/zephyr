/*
 * Copyright (c) 2019 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power.h>
#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_cortex.h>
#include <stm32l4xx_ll_pwr.h>
#include <stm32l4xx_ll_rcc.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Invoke Low Power/System Off specific Tasks */
void sys_set_power_state(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
 #ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_1
	case SYS_POWER_STATE_SLEEP_1:

		/* this corresponds to the STOP0 mode: */
#ifdef CONFIG_DEBUG
		/* Enable the Debug Module during STOP mode */
		LL_DBGMCU_EnableDBGStopMode();
#endif
		/* enable Power clock */
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
		/* ensure HSI is the wake-up system clock */
		LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
		/* enter STOP0 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0);
		LL_LPM_EnableDeepSleep();
		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();
		break;
 #endif
 #ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_2
	case SYS_POWER_STATE_SLEEP_2:
		/* this corresponds to the STOP1 mode: */
#ifdef CONFIG_DEBUG
		/* Enable the Debug Module during STOP mode */
		LL_DBGMCU_EnableDBGStopMode();
#endif
		/* enable Power clock */
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
		/* ensure HSI is the wake-up system clock */
		LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
		/* enter STOP1 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);
		LL_LPM_EnableDeepSleep();
		k_cpu_idle();
		break;
 #endif
 #ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_3
	case SYS_POWER_STATE_SLEEP_3:
		/* this corresponds to the STOP2 mode: */
#ifdef CONFIG_DEBUG
		/* Enable the Debug Module during STOP mode */
		LL_DBGMCU_EnableDBGStopMode();
#endif
		/* enable Power clock */
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
		/* ensure HSI is the wake-up system clock */
		LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
		LL_PWR_DisableSRAM3Retention();
		/* enter STOP2 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP2);
		LL_LPM_EnableDeepSleep();
		k_cpu_idle();
		break;
 #endif
#endif /* CONFIG_SYS_POWER_SLEEP_STATES */
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP_STATES
 #ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_1
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		/* this corresponds to the STANDBY mode: */
#ifdef CONFIG_DEBUG
		/* Enable the Debug Module during Standy mode */
		LL_DBGMCU_EnableDBGStandbyMode();
#endif
		/* enable Power clock */
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
		/* clear WUFx bits in the power status register 1 */
		LL_PWR_ClearFlag_WU();
		/* the content of SRAM2 will be lost */
		LL_PWR_DisableSRAM2Retention();
		/* set all GPIO to analog mode and disable GPIO clocks */
		/* TODO */
		/* program the wakeUp source RTC or wdg or wakeUp pin */
		/* TODO */
		/* enter STANDBY mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
		LL_LPM_EnableDeepSleep();
		k_cpu_idle();
		break;
 #endif
 #ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_2
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		/* this corresponds to the SHUTDOWN mode */
		/* enable Power clock */
		LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
		/* clear WUFx bits in the power status register 1 */
		LL_PWR_ClearFlag_WU();
		/* set all GPIO to analog mode and disable GPIO clocks */
		/* TODO */
		/* program the wakeUp source RTC or wdg or wakeUp pin */
		/* TODO */
		/* enter SHUTDOWN mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);
		LL_LPM_EnableDeepSleep();
		k_cpu_idle();
		break;
 #endif
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP_STATES */
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void _sys_pm_power_state_exit_post_ops(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
 #ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_1
	case SYS_POWER_STATE_SLEEP_1:
		LL_LPM_DisableSleepOnExit();
		break;
 #endif
 #ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_2
	case SYS_POWER_STATE_SLEEP_2:
		LL_LPM_DisableSleepOnExit();
		break;
 #endif
 #ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_3
	case SYS_POWER_STATE_SLEEP_3:
		LL_LPM_DisableSleepOnExit();
		break;
 #endif
#endif /* CONFIG_SYS_POWER_SLEEP_STATES */
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP_STATES
 #ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_1
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		LL_PWR_DisableLowPowerRunMode();
		break;
 #endif
 #ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_2
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		LL_PWR_DisableLowPowerRunMode();
		break;
 #endif
#endif /* CONFIG_SYS_POWER_DEEP_SLEEP_STATES */
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}
