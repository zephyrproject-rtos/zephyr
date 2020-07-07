/*
 * Copyright (c) 2019 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power.h>
#include <soc.h>
#include <init.h>

#include <stm32l1xx_ll_bus.h>
#include <stm32l1xx_ll_cortex.h>
#include <stm32l1xx_ll_pwr.h>
#include <stm32l1xx_ll_rcc.h>
#include <stm32l1xx_ll_rtc.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);


/* #ifdef RTC_CLOCK_SOURCE_LSI */
/* ck_apre=LSIFreq/(ASYNC prediv + 1) with LSIFreq=37 kHz RC */
#define RTC_ASYNCH_PREDIV          ((uint32_t)0x7F)
/* ck_spre=ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV           ((uint32_t)0x122)

/* Invoke Low Power/System Off specific Tasks */
void sys_set_power_state(enum power_states state)
{

	LL_RTC_ClearFlag_WUT(RTC);

	switch (state) {
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_1
	case SYS_POWER_STATE_SLEEP_1:

		/* this corresponds to the STOP0 mode: */
#ifdef CONFIG_DEBUG
		/* Enable the Debug Module during STOP mode */
		LL_DBGMCU_EnableDBGStopMode();
#endif /* CONFIG_DEBUG */
		/* Enable ultra low power mode */
		LL_PWR_EnableUltraLowPower();
		/* Set the regulator to low power before setting MODE_STOP.
		 * If the regulator remains in "main mode",
		 * it consumes more power without providing any additional feature. */
		LL_PWR_SetRegulModeLP(LL_PWR_REGU_LPMODES_LOW_POWER);

		/* enter STOP0 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP);
		LL_LPM_EnableDeepSleep();
		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();
		break;
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_1 */
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
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_1 */
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_2
	case SYS_POWER_STATE_SLEEP_2:
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_2 */
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_3
	case SYS_POWER_STATE_SLEEP_3:
#endif /* CONFIG_HAS_SYS_POWER_STATE_SLEEP_3 */
		LL_LPM_DisableSleepOnExit();
		break;
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

static void rtc_irq_handler(struct device *unused)
{

	ARG_UNUSED(unused);

	LL_RTC_ClearFlag_WUT(RTC);

}

/* Initialize STM32 Power */
static int stm32_power_init(struct device *dev)
{
	unsigned int ret;

	ARG_UNUSED(dev);

	ret = irq_lock();

	/* enable PWR clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	/* enable RTC clocked by LSI */
	LL_PWR_EnableBkUpAccess();
	if (LL_RCC_LSI_IsReady() == 0) {
		LL_RCC_ForceBackupDomainReset();
		LL_RCC_ReleaseBackupDomainReset();
		LL_RCC_LSI_Enable();
		while (LL_RCC_LSI_IsReady() != 1) {
		}
	}
	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
	/* Enable RTC Clock */
	LL_RCC_EnableRTC();

	/* configure RTC */
	/* Disable RTC registers write protection */
	LL_RTC_DisableWriteProtection(RTC);
	/* Set Initialization mode */
	LL_RTC_EnableInitMode(RTC);
	/* Check if the Initialization mode is set */
	while (LL_RTC_IsActiveFlag_INIT(RTC) != 1) {
	}
	/* Set prescaler according to source clock */
	LL_RTC_SetAsynchPrescaler(RTC, RTC_ASYNCH_PREDIV);
	LL_RTC_SetSynchPrescaler(RTC, RTC_SYNCH_PREDIV);

	/* Disable wake up timer to modify it */
	LL_RTC_WAKEUP_Disable(RTC);
	LL_RTC_ClearFlag_WUT(RTC);

	/* Clear the event flag and possible pending interrupt */
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(rtc)),
		    DT_IRQ(DT_NODELABEL(rtc), priority),
		    rtc_irq_handler, 0, 0);

	irq_enable(DT_IRQN(DT_NODELABEL(rtc)));
#ifdef CONFIG_DEBUG
	LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_RTC_STOP);
#endif /* CONFIG_DEBUG */
	/* Exit Initialization mode */
	LL_RTC_DisableInitMode(RTC);
	/* Clear RSF flag */
	LL_RTC_ClearFlag_RS(RTC);
	/* Wait the registers to be synchronised */
	while(LL_RTC_IsActiveFlag_RS(RTC) != 1) {
	}

	LL_RTC_EnableWriteProtection(RTC);

	irq_unlock(ret);

	return 0;
}

SYS_INIT(stm32_power_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
