/*
 * Copyright (c) 2026 Hula Earth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <clock_control/clock_stm32_ll_common.h>
#include <soc.h>

#include <stm32_ll_cortex.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32n6xx_hal_pwr.h>
#include <stm32n6xx_hal_pwr_ex.h>
#include <stm32n6xx_hal_rcc.h>

#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#if DT_HAS_CHOSEN(zephyr_system_timer_companion)
#define SYSTEM_TIMER_COMPANION_NODE DT_CHOSEN(zephyr_system_timer_companion)
#else
/* Compatibility fallback for the deprecated /chosen/zephyr,cortex-m-idle-timer.
 * Scheduled for removal in Zephyr 4.6.0.
 */
#define SYSTEM_TIMER_COMPANION_NODE DT_CHOSEN(zephyr_cortex_m_idle_timer)
#endif

BUILD_ASSERT(DT_SAME_NODE(SYSTEM_TIMER_COMPANION_NODE, DT_NODELABEL(rtc)),
		"STM32N6x needs RTC as the system timer companion for power management");

static void stm32n6_prepare_stop(void)
{
	/*
	 * The RTC companion schedules normal PM wakeups; enabled EXTI interrupts are
	 * independent asynchronous wake sources.
	 */
	LL_LPM_DisableEventOnPend();
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_STOPF);

	/*
	 * STOP resumes on HSI. Keep that source available and make the wake clock
	 * selection explicit instead of relying on reset defaults.
	 */
	__HAL_RCC_HSISTOP_ENABLE();
	LL_RCC_SetSysWakeUpClkSource(LL_RCC_SYSWAKEUP_CLKSOURCE_HSI);

	/*
	 * STOP voltage is separate from the run-mode VOS. Scale 3 is the validated
	 * conservative N6 STOP setting.
	 */
	(void)HAL_PWREx_ControlStopModeVoltageScaling(PWR_REGULATOR_STOP_VOLTAGE_SCALE3);
	LL_PWR_SetPowerDownModeDS(LL_PWR_POWERDOWN_MODE_DS_STOP);
	LL_LPM_EnableDeepSleep();
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	if (state != PM_STATE_SUSPEND_TO_IDLE || substate_id != 1U) {
		LOG_DBG("Unsupported power state %u substate-id %u", state, substate_id);
		return;
	}

	stm32n6_prepare_stop();

	/*
	 * PM invokes this hook with interrupts locked. k_cpu_idle() therefore enters
	 * WFI without allowing a wake ISR to run before post operations.
	 */
	k_cpu_idle();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	if (state == PM_STATE_SUSPEND_TO_IDLE && substate_id == 1U) {
		LL_LPM_DisableSleepOnExit();
		LL_LPM_EnableSleep();

		/*
		 * STM32N6 STOP returns with the app clock tree unavailable. Restore the
		 * configured safe run point before the kernel timer exit hook or the wake
		 * ISR can execute normal application code.
		 */
		int ret = stm32_clock_control_init(NULL);

		if (ret != 0) {
			/*
			 * Do not log from the wake ISR. Continuing with an unknown clock tree is
			 * unsafe. Halt unconditionally if restoration fails.
			 */
			__ASSERT_NO_MSG(ret == 0);
			__disable_irq();
			while (true) {
				__WFI();
			}
		}

		/*
		 * The N6 SoC enables both caches for normal execution. Reassert that
		 * invariant only after RCC restoration; cache management is a no-op when
		 * the corresponding Kconfig option is disabled.
		 */
		sys_cache_instr_enable();
		sys_cache_data_enable();
	}

	/* PM entered this hook with interrupts locked. */
	irq_unlock(0);
}
