/*
 * Copyright (c) 2019 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <pm/pm.h>
#include <soc.h>
#include <init.h>

#include <stm32l4xx_ll_utils.h>
#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_cortex.h>
#include <stm32l4xx_ll_pwr.h>
#include <stm32l4xx_ll_rcc.h>
#include <stm32l4xx_ll_system.h>
#include <clock_control/clock_stm32_ll_common.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* select MSI as wake-up system clock if configured, HSI otherwise */
#if STM32_SYSCLK_SRC_MSI
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_MSI
#else
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_HSI
#endif

static void pm_state_set_soft_off(uint8_t substate_id)
{
	switch (substate_id) {
	case 1: /* this corresponds to the Shutdown mode: */
		/* Disable Pin 1 which is used as wakeup source */
		LL_PWR_DisableWakeUpPin(LL_PWR_WAKEUP_PIN1);
		/* Clear all wake up Flag */
		LL_PWR_ClearFlag_WU();
		/* Enable wakeup pin */
		LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN1);

		/* Set SHUTDOWN mode when CPU enters deepsleep */
		LL_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);
		/* Set SLEEPDEEP bit of Cortex System Control Register */
		LL_LPM_EnableDeepSleep();

		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();

	default:
		LOG_ERR("Unsupported power state  substate-id [%u]", substate_id);
		break;
	}
}

static void pm_state_set_suspend_to_idle(uint8_t substate_id)
{
	switch (substate_id) {
	case 1: /* this corresponds to the STOP0 mode: */
		/* ensure the proper wake-up system clock */
		LL_RCC_SetClkAfterWakeFromStop(RCC_STOP_WAKEUPCLOCK_SELECTED);
		/* enter STOP0 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0);
		LL_LPM_EnableDeepSleep();
		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();
		break;
	case 2: /* this corresponds to the STOP1 mode: */
		/* ensure the proper wake-up system clock */
		LL_RCC_SetClkAfterWakeFromStop(RCC_STOP_WAKEUPCLOCK_SELECTED);
		/* enter STOP1 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);
		LL_LPM_EnableDeepSleep();
		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();
		break;
	case 3: /* this corresponds to the STOP2 mode: */
		/* ensure the proper wake-up system clock */
		LL_RCC_SetClkAfterWakeFromStop(RCC_STOP_WAKEUPCLOCK_SELECTED);
#ifdef PWR_CR1_RRSTP
		LL_PWR_DisableSRAM3Retention();
#endif /* PWR_CR1_RRSTP */
		/* enter STOP2 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP2);
		LL_LPM_EnableDeepSleep();
		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();
		break;
	default:
		LOG_ERR("Unsupported power state substate-id [%u]", substate_id);
		break;
	}
}

/* Invoke Low Power/System Off specific Tasks */
void pm_power_state_set(struct pm_state_info info)
{
	switch (info.state) {
	case PM_STATE_SOFT_OFF:
		pm_state_set_soft_off(info.substate_id);
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		pm_state_set_suspend_to_idle(info.substate_id);
		break;
	default:
		LOG_ERR("Unsupported power state state-id [%u]", info.state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	if (info.state != PM_STATE_SUSPEND_TO_IDLE) {
		LOG_DBG("Unsupported power substate-id %u", info.state);
	} else {
		switch (info.substate_id) {
		case 1: /* STOP0 */
			__fallthrough;
		case 2: /* STOP1 */
			__fallthrough;
		case 3: /* STOP2 */
			LL_LPM_DisableSleepOnExit();
			LL_LPM_EnableSleep();
			break;
		default:
			LOG_DBG("Unsupported power substate-id %u",
				info.substate_id);
			break;
		}
		/* need to restore the clock */
		stm32_clock_control_init(NULL);
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
	ARG_UNUSED(dev);

	/* enable Power clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

#ifdef CONFIG_DEBUG
	/* Enable the Debug Module during STOP mode */
	LL_DBGMCU_EnableDBGStopMode();
#endif /* CONFIG_DEBUG */

	return 0;
}

SYS_INIT(stm32_power_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
