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



void config_wakeup_features(void)
{	
	// Configure wake-up features
	// WKUP2(PC13) only ,  - active low, pull-up
	PWR->PUCRC = PWR_PUCRC_PC13; // Set pull-ups for standby modes		
	PWR->CR4 = 0; // Set wakeup pins' polarity to High level (rising edge)
	//PWR->CR4 = PWR_CR4_WP2; // Set wakeup pins' polarity to low level (falling edge)
	PWR->CR3 = PWR_CR3_APC  | PWR_CR3_EWUP2; // Enable pin pull configurations and wakeup pins 
	PWR->SCR = PWR_SCR_CWUF; // Clear wakeup flags
}

void Enter_low_power_mode(void)
{
	(void)PWR->CR1; // Ensure that the previous PWR register operations have been completed

	// Configure CPU core
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; // Enable CPU deep sleep mode
#ifdef NDEBUG
	DBGMCU->CR = 0; // Disable debug, trace and IWDG in low-power modes
#endif

	// Enter low-power mode
	for (;;) {
		__DSB();
		__WFI();
		__ISB(); 

	}
}

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

void set_mode_standby()
{	
	/* Select standby mode */
	printk(" mode_standby done\n\n\n");
	config_wakeup_features();			
	LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
	Enter_low_power_mode();
}

void set_mode_shutdown()
{	
	/* Select shutdown mode */
	printk(" mode_shutdown done\n\n\n");
	config_wakeup_features();		
	//printk(" PWR->CR1 =%x \n",PWR->CR1);//remove
	LL_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN); // no difference beetween shutdown et standby for bit PWR_SR1_SBF !	
	//printk(" PWR->CR1 =%x \n\n\n",PWR->CR1);//remove
	Enter_low_power_mode();
}

/* Invoke Low Power/System Off specific Tasks */
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		set_mode_stop(substate_id);
		break;
	case PM_STATE_STANDBY:
		/* To be tested */		
		set_mode_standby();
		break;
	case PM_STATE_SOFT_OFF:						
		set_mode_shutdown();		
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}
	/* Set SLEEPDEEP bit of Cortex System Control Register */
	LL_LPM_EnableDeepSleep();

	/* Select mode entry : WFE or WFI and enter the CPU selected mode */
	k_cpu_idle();
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
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
		/* To be tested */
		//LL_LPM_EnableSleep();
	case PM_STATE_SOFT_OFF:
		/* We should not get there */		
		__fallthrough;
	case PM_STATE_ACTIVE:
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
static int stm32_power_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* enable Power clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

#ifdef CONFIG_DEBUG
	/* Enable the Debug Module during STOP mode */
	LL_DBGMCU_EnableDBGStopMode();
#endif /* CONFIG_DEBUG */

	return 0;
}

SYS_INIT(stm32_power_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
