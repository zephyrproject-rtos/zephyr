/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <zephyr/init.h>

#include <stm32u5xx_ll_utils.h>
#include <stm32u5xx_ll_bus.h>
#include <stm32u5xx_ll_cortex.h>
#include <stm32u5xx_ll_pwr.h>
#include <stm32u5xx_ll_icache.h>
#include <stm32u5xx_ll_rcc.h>
#include <stm32u5xx_ll_system.h>
#include <clock_control/clock_stm32_ll_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* select MSI as wake-up system clock if configured, HSI otherwise */
#if STM32_SYSCLK_SRC_MSI
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_MSI
#else
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_HSI
#endif

#ifdef CONFIG_STM32_STOP3_LP_MODE
static void pwr_stop3_isr(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Clear all wake-up flags */
	LL_PWR_ClearFlag_WU();
}

static void disable_cache(void)
{
	/* Disabling ICACHE */
	LL_ICACHE_Disable();
	while (LL_ICACHE_IsEnabled() == 1U) {
	}

	/* Wait until ICACHE_SR.BUSYF is cleared */
	while (LL_ICACHE_IsActiveFlag_BUSY() == 1U) {
	}

	/* Wait until ICACHE_SR.BSYENDF is set */
	while (LL_ICACHE_IsActiveFlag_BSYEND() == 0U) {
	}
}
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
	case 3: /* enter STOP2 mode */
		LL_PWR_SetPowerMode(LL_PWR_STOP2_MODE);
		break;
#ifdef CONFIG_STM32_STOP3_LP_MODE
	case 4: /* enter STOP3 mode */

		LL_PWR_SetSRAM2SBRetention(LL_PWR_SRAM2_SB_FULL_RETENTION);
		/* Enable RTC wakeup
		 * This configures an internal pin that generates an event to wakeup the system
		 */
		LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN7);
		LL_PWR_SetWakeUpPinSignal3Selection(LL_PWR_WAKEUP_PIN7);

		/* Clear flags */
		LL_PWR_ClearFlag_SB();
		LL_PWR_ClearFlag_WU();

		disable_cache();

		LL_PWR_SetPowerMode(LL_PWR_STOP3_MODE);
		break;
#endif
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		break;
	}
}

void set_mode_standby(uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	/* Select standby mode */
	LL_PWR_SetPowerMode(LL_PWR_STANDBY_MODE);
}

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		set_mode_stop(substate_id);
		break;
	case PM_STATE_STANDBY:
		/* To be tested */
		set_mode_standby(substate_id);
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
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		if (substate_id <= 3) {
			LL_LPM_DisableSleepOnExit();
			LL_LPM_EnableSleep();
#ifdef CONFIG_STM32_STOP3_LP_MODE
		} else if (substate_id == 4) {
			stm32_clock_control_standby_exit();

			LL_ICACHE_SetMode(LL_ICACHE_1WAY);
			LL_ICACHE_Enable();
			while (LL_ICACHE_IsEnabled() == 0U) {
			}

			LL_LPM_DisableSleepOnExit();
			LL_LPM_EnableSleep();
#endif
		} else {
			LOG_DBG("Unsupported power substate-id %u",
							substate_id);
		}
	case PM_STATE_STANDBY:
		/* To be tested */
		LL_LPM_EnableSleep();
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

	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}

/* Initialize STM32 Power */
void stm32_power_init(void)
{

	/* enable Power clock */
	LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_PWR);

#ifdef CONFIG_STM32_STOP3_LP_MODE
	IRQ_CONNECT(PWR_S3WU_IRQn, 0,
		    pwr_stop3_isr, 0, 0);
	irq_enable(PWR_S3WU_IRQn);
#endif
}
