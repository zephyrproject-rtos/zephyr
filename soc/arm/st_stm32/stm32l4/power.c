/*
 * Copyright (c) 2019 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power.h>
#include <soc.h>
#include <init.h>
#include <spinlock.h>

#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_cortex.h>
#include <stm32l4xx_ll_pwr.h>
#include <stm32l4xx_ll_rcc.h>
#include <stm32l4xx_ll_lptim.h>
#include <stm32l4xx_ll_system.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#ifdef CONFIG_SYS_PM_POLICY_RESIDENCY_STM32
static struct k_spinlock lock;

static void lptim_irq_handler(struct device *unused)
{
	ARG_UNUSED(unused);

	if ((LL_LPTIM_IsActiveFlag_ARRM(LPTIM1) != 0)
		&& LL_LPTIM_IsEnabledIT_ARRM(LPTIM1) != 0) {
		/* when irq occurs, counter is already set to 0 */
		k_spinlock_key_t key = k_spin_lock(&lock);

		/* do not change ARR yet, z_clock_announce will do */
		LL_LPTIM_ClearFLAG_ARRM(LPTIM1);

		k_spin_unlock(&lock, key);
	}
}
#endif /* CONFIG_SYS_PM_POLICY_RESIDENCY_STM32 */

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
#endif /* CONFIG_DEBUG */
		/* ensure HSI is the wake-up system clock */
		LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
		/* enter STOP0 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0);
		LL_LPM_EnableDeepSleep();
		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();
		break;
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_1 */
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_2
	case SYS_POWER_STATE_SLEEP_2:
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
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_2 */
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_3
	case SYS_POWER_STATE_SLEEP_3:
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
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_3 */
#endif /* CONFIG_SYS_POWER_SLEEP_STATES */
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
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_1 */
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_2
	case SYS_POWER_STATE_SLEEP_2:
		LL_LPM_DisableSleepOnExit();
		break;
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_2 */
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_3
	case SYS_POWER_STATE_SLEEP_3:
		LL_LPM_DisableSleepOnExit();
		break;
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_3 */
#endif /* CONFIG_SYS_POWER_SLEEP_STATES */
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

#ifdef CONFIG_SYS_PM_POLICY_RESIDENCY_STM32
/* Initialize STM32 LPTIM */
static void stm32_lptim_init(void)
{
	/* enable LPTIM clock source */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LPTIM1);
	LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_LPTIM1);

#ifdef CONFIG_STM32_LPTIM_CLOCK_LSI
	/* enable LSI clock */
#ifdef CONFIG_SOC_SERIES_STM32WBX
	LL_RCC_LSI1_Enable();
	while (!LL_RCC_LSI1_IsReady()) {
#else
	LL_RCC_LSI_Enable();
	while (!LL_RCC_LSI_IsReady()) {
#endif /* CONFIG_SOC_SERIES_STM32WBX */
		/* Wait for LSI ready */
	}

	LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSI);

#else /* CONFIG_STM32_LPTIM_CLOCK_LSI */
#ifdef LL_APB1_GRP1_PERIPH_PWR
	/* Enable the power interface clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
#endif /* LL_APB1_GRP1_PERIPH_PWR */
	/* enable backup domain */
	LL_PWR_EnableBkUpAccess();

	/* enable LSE clock */
	LL_RCC_LSE_DisableBypass();
	LL_RCC_LSE_Enable();
	while (!LL_RCC_LSE_IsReady()) {
		/* Wait for LSE ready */
	}
	LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSE);

#endif /* CONFIG_STM32_LPTIM_CLOCK_LSI */

	/* Clear the event flag and possible pending interrupt */
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(lptim1)),
		    DT_IRQ(DT_NODELABEL(lptim1), priority),
		    lptim_irq_handler, 0, 0);
	irq_enable(DT_IRQN(DT_NODELABEL(lptim1)));

	/* configure the LPTIM1 counter */
	LL_LPTIM_SetClockSource(LPTIM1, LL_LPTIM_CLK_SOURCE_INTERNAL);
	/*
	 * configure the LPTIM1 prescaler with default value of 8
	 * this will allow sleep period from 0 to 16000ms
	 */
	LL_LPTIM_SetPrescaler(LPTIM1, LL_LPTIM_PRESCALER_DIV8);
	LL_LPTIM_SetPolarity(LPTIM1, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
	LL_LPTIM_SetUpdateMode(LPTIM1, LL_LPTIM_UPDATE_MODE_IMMEDIATE);
	LL_LPTIM_SetCounterMode(LPTIM1, LL_LPTIM_COUNTER_MODE_INTERNAL);
	LL_LPTIM_DisableTimeout(LPTIM1);
	/* counting start is initiated by software */
	LL_LPTIM_TrigSw(LPTIM1);

	/* LPTIM1 interrupt set-up before enabling */
	/* no Compare match Interrupt */
	LL_LPTIM_DisableIT_CMPM(LPTIM1);
	LL_LPTIM_ClearFLAG_CMPM(LPTIM1);

	/* Autoreload match Interrupt */
	LL_LPTIM_EnableIT_ARRM(LPTIM1);
	LL_LPTIM_ClearFLAG_ARRM(LPTIM1);
	/* ARROK bit validates the write operation to ARR register */
	LL_LPTIM_ClearFlag_ARROK(LPTIM1);
	LL_LPTIM_DisableIT_ARROK(LPTIM1);
#ifdef CONFIG_DEBUG
	/* stop LPTIM1 during DEBUG */
	LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_LPTIM1_STOP);
#endif
	/*  Enable the LPTIM1 counter */
	LL_LPTIM_Enable(LPTIM1);

}
#endif /* CONFIG_SYS_PM_POLICY_RESIDENCY_STM32 */

/* Initialize STM32 Power */
static int stm32_power_init(struct device *dev)
{
	unsigned int ret;

	ARG_UNUSED(dev);

	ret = irq_lock();

	/* enable Power clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	/* LPTIM initalization */
	if (IS_ENABLED(CONFIG_SYS_PM_POLICY_RESIDENCY_STM32)) {
		stm32_lptim_init();
	}
	irq_unlock(ret);

	return 0;
}

SYS_INIT(stm32_power_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
