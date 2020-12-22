/*
 * Copyright (c) 2019 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power.h>
#include <soc.h>
#include <init.h>

#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_cortex.h>
#include <stm32l4xx_ll_pwr.h>
#include <stm32l4xx_ll_rcc.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Invoke Low Power/System Off specific Tasks */
void pm_power_state_set(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_PM_SLEEP_STATES
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_1
	case POWER_STATE_SLEEP_1:

		/* this corresponds to the STOP0 mode: */
#ifdef CONFIG_DEBUG
		/* Enable the Debug Module during STOP mode */
		LL_DBGMCU_EnableDBGStopMode();
#endif /* CONFIG_DEBUG */
		/* ensure HSI is the wake-up system clock */
		LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
		/* enter STOP0 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0);
		LL_LPM_EnableDeepSleep();
		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();
		break;
#endif /* CONFIG_HAS_POWER_STATE_SLEEP_1 */
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_2
	case POWER_STATE_SLEEP_2:
		/* this corresponds to the STOP1 mode: */
#ifdef CONFIG_DEBUG
		/* Enable the Debug Module during STOP mode */
		LL_DBGMCU_EnableDBGStopMode();
#endif /* CONFIG_DEBUG */
		/* ensure HSI is the wake-up system clock */
		LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
		/* enter STOP1 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);
		LL_LPM_EnableDeepSleep();
		k_cpu_idle();
		break;
#endif /* CONFIG_HAS_POWER_STATE_SLEEP_2 */
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_3
	case POWER_STATE_SLEEP_3:
		/* this corresponds to the STOP2 mode: */
#ifdef CONFIG_DEBUG
		/* Enable the Debug Module during STOP mode */
		LL_DBGMCU_EnableDBGStopMode();
#endif /* CONFIG_DEBUG */
		/* ensure HSI is the wake-up system clock */
		LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
#ifdef PWR_CR1_RRSTP
		LL_PWR_DisableSRAM3Retention();
#endif /* PWR_CR1_RRSTP */
		/* enter STOP2 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP2);
		LL_LPM_EnableDeepSleep();
		k_cpu_idle();
		break;
#endif /* CONFIG_HAS_POWER_STATE_SLEEP_3 */
#endif /* CONFIG_PM_SLEEP_STATES */
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_PM_SLEEP_STATES
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_1
	case POWER_STATE_SLEEP_1:
#endif /* CONFIG_HAS_POWER_STATE_SLEEP_1 */
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_2
	case POWER_STATE_SLEEP_2:
#endif /* CONFIG_HAS_POWER_STATE_SLEEP_2 */
#ifdef CONFIG_HAS_POWER_STATE_SLEEP_3
	case POWER_STATE_SLEEP_3:
#endif /* CONFIG_HAS_POWER_STATE_SLEEP_3 */
		LL_LPM_DisableSleepOnExit();
		LL_LPM_EnableSleep();
		break;
#endif /* CONFIG_PM_SLEEP_STATES */
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

/* Initialize STM32 Power */
static int stm32_power_init(const struct device *dev)
{
	unsigned int ret;

	ARG_UNUSED(dev);

	ret = irq_lock();

	/* enable Power clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	irq_unlock(ret);

	return 0;
}

SYS_INIT(stm32_power_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
