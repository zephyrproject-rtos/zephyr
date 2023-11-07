/*
 * Copyright (c) 2022 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/drivers/timer/system_timer.h>

#include <stm32wbaxx_ll_bus.h>
#include <stm32wbaxx_ll_cortex.h>
#include <stm32wbaxx_ll_pwr.h>
#include <stm32wbaxx_ll_icache.h>
#include <stm32wbaxx_ll_rcc.h>
#include <stm32wbaxx_ll_system.h>
#include <clock_control/clock_stm32_ll_common.h>

#ifdef CONFIG_BT_STM32WBA
#include "scm.h"
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

static int stm32_power_init(void);

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

static void set_mode_stop(uint8_t substate_id)
{

	LL_PWR_ClearFlag_STOP();
	LL_RCC_ClearResetFlags();

	/* Erratum 2.2.15:
	 * Disabling ICACHE is required before entering stop mode
	 */
	disable_cache();

#ifdef CONFIG_BT_STM32WBA
	scm_setwaitstates(LP);
#endif
	/* Set SLEEPDEEP bit of Cortex System Control Register */
	LL_LPM_EnableDeepSleep();

	while (LL_PWR_IsActiveFlag_ACTVOS() == 0) {
	}

	switch (substate_id) {
	case 1: /* enter STOP0 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0);
		break;
	case 2: /* enter STOP1 mode */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);
		break;
	default:
		LOG_DBG("Unsupported power state substate-id %u", substate_id);
		break;
	}
}

#if defined(CONFIG_PM_S2RAM)
static int suspend_to_ram(void)
{
	LL_LPM_EnableDeepSleep();

	while (LL_PWR_IsActiveFlag_ACTVOS() == 0) {
	}

	/* Select mode entry : WFE or WFI and enter the CPU selected mode */
	k_cpu_idle();

	return 0;
}

static void set_mode_suspend_to_ram(void)
{
	/* Enable SRAM full retention */
	LL_PWR_SetSRAM1SBRetention(LL_PWR_SRAM1_SB_FULL_RETENTION);
	LL_PWR_SetSRAM2SBRetention(LL_PWR_SRAM2_SB_FULL_RETENTION);

	/* Enable RTC wakeup
	 * This configures an internal pin that generates an event to wakeup the system
	 */
	LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN7);
	LL_PWR_SetWakeUpPinSignal3Selection(LL_PWR_WAKEUP_PIN7);

	/* Clear flags */
	LL_PWR_ClearFlag_SB();
	LL_PWR_ClearFlag_WU();
	LL_RCC_ClearResetFlags();

	disable_cache();

	/* Select standby mode */
	LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);

	/* Save context and enter Standby mode */
	arch_pm_s2ram_suspend(suspend_to_ram);

	/* Execution is restored at this point after wake up */
	/* Restore system clock as soon as we exit standby mode */
	sys_clock_idle_exit();
}
#endif

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		set_mode_stop(substate_id);

		/* Select mode entry : WFE or WFI and enter the CPU selected mode */
		k_cpu_idle();

		break;
#if defined(CONFIG_PM_S2RAM)
	case PM_STATE_SUSPEND_TO_RAM:
		set_mode_suspend_to_ram();
		break;
#endif
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
#ifdef CONFIG_BT_STM32WBA
	if (LL_PWR_IsActiveFlag_STOP() == 1U) {
		scm_setup();
	} else {
		scm_setwaitstates(RUN);
	}
#endif

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		if (substate_id <= 2) {
			/* Erratum 2.2.15:
			 * Enable ICACHE when exiting stop mode
			 */
			LL_ICACHE_SetMode(LL_ICACHE_1WAY);
			LL_ICACHE_Enable();
			while (LL_ICACHE_IsEnabled() == 0U) {
			}

			LL_LPM_DisableSleepOnExit();
			LL_LPM_EnableSleep();
		} else {
			LOG_DBG("Unsupported power substate-id %u",
							substate_id);
		}
		break;
	case PM_STATE_SUSPEND_TO_RAM:
#if defined(CONFIG_PM_S2RAM)
		stm32wba_init();
		stm32_power_init();

		LL_LPM_DisableSleepOnExit();
		LL_LPM_EnableSleep();
#else
		LOG_DBG("Suspend to RAM needs CONFIG_PM_S2RAM to be enabled");
#endif
		break;
	case PM_STATE_STANDBY:
		__fallthrough;
	case PM_STATE_SUSPEND_TO_DISK:
		__fallthrough;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	/* When BLE is enabled, clock restoration is performed by SCM */
#if !defined(CONFIG_BT_STM32WBA)
	stm32_clock_control_init(NULL);
#endif

	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}

/* Initialize STM32 Power */
static int stm32_power_init(void)
{

#ifdef CONFIG_BT_STM32WBA
	scm_init();
#endif

	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_PWR);

#ifdef CONFIG_DEBUG
	LL_DBGMCU_EnableDBGStandbyMode();
	LL_DBGMCU_APB7_GRP1_FreezePeriph(LL_DBGMCU_APB7_GRP1_RTC_STOP);
	LL_DBGMCU_APB7_GRP1_FreezePeriph(LL_DBGMCU_APB7_GRP1_LPTIM1_STOP);
#else
	LL_DBGMCU_DisableDBGStandbyMode();
#endif

	/* Enabling  Ultra Low power mode */
	LL_PWR_EnableUltraLowPowerMode();

	LL_FLASH_EnableSleepPowerDown();

	return 0;
}

SYS_INIT(stm32_power_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
