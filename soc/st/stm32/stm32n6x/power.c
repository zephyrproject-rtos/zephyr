/*
 * Copyright (c) 2026 Hula Earth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <clock_control/clock_stm32_ll_common.h>
#include <soc.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <errno.h>

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

#define STM32N6_CLOCK_TIMEOUT_US 100000U

static uint32_t stm32n6_cpu_clock_source(void)
{
#if defined(STM32_CPUCLK_SRC_HSI)
	return LL_RCC_CPU_CLKSOURCE_STATUS_HSI;
#elif defined(STM32_CPUCLK_SRC_MSI)
	return LL_RCC_CPU_CLKSOURCE_STATUS_MSI;
#elif defined(STM32_CPUCLK_SRC_HSE)
	return LL_RCC_CPU_CLKSOURCE_STATUS_HSE;
#elif defined(STM32_CPUCLK_SRC_IC1)
	return LL_RCC_CPU_CLKSOURCE_STATUS_IC1;
#else
	return UINT32_MAX;
#endif
}

static void stm32n6_prepare_stop(void)
{
	/*
	 * The RTC companion schedules normal PM wakeups; enabled EXTI interrupts are
	 * independent asynchronous wake sources.
	 */
	LL_LPM_DisableEventOnPend();
	LL_LPM_DisableSleepOnExit();
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_STOPF);
	__HAL_RCC_PWR_CLK_ENABLE();
	HAL_PWR_EnableBkUpAccess();

	/* Match the HAL reference's active BSEC clock enable before STOP. */
	__HAL_RCC_BSEC_CLK_ENABLE();
	__HAL_RCC_RTC_ENABLE();
	__HAL_RCC_RTCAPB_CLK_ENABLE();

	/* Keep the secure wake-source register banks clocked throughout
	 * STOP.
	 */
	__HAL_RCC_BSEC_CLK_SLEEP_ENABLE();
	__HAL_RCC_GPIOA_CLK_SLEEP_ENABLE();
	__HAL_RCC_SYSCFG_CLK_SLEEP_ENABLE();
	__HAL_RCC_RTC_CLK_SLEEP_ENABLE();
	__HAL_RCC_RTCAPB_CLK_SLEEP_ENABLE();

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

static int stm32n6_switch_to_hsi(void)
{
	if (LL_RCC_HSI_IsReady() == 0U) {
		LL_RCC_HSI_Enable();
		if (!WAIT_FOR(LL_RCC_HSI_IsReady(), STM32N6_CLOCK_TIMEOUT_US,
			      k_busy_wait(1))) {
			return -ETIMEDOUT;
		}
	}

	LL_RCC_SetCpuClkSource(LL_RCC_CPU_CLKSOURCE_HSI);
	if (!WAIT_FOR(LL_RCC_GetCpuClkSource() ==
		      LL_RCC_CPU_CLKSOURCE_STATUS_HSI, STM32N6_CLOCK_TIMEOUT_US,
		      k_busy_wait(1))) {
		return -ETIMEDOUT;
	}
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
	if (!WAIT_FOR(LL_RCC_GetSysClkSource() ==
		      LL_RCC_SYS_CLKSOURCE_STATUS_HSI, STM32N6_CLOCK_TIMEOUT_US,
		      k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	return 0;
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	unsigned int key;

	if (state != PM_STATE_SUSPEND_TO_IDLE || substate_id != 1U) {
		LOG_DBG("Unsupported power state %u substate-id %u", state, substate_id);
		return;
	}

	/* The HAL reference moves CPU/SYSCLK to HSI before STOP because the PLL
	 * clock tree is unavailable in the low-power state.
	 */
	if (stm32n6_switch_to_hsi() != 0) {
		return;
	}
	stm32n6_prepare_stop();
	/* Match the STM32 PM contract: preserve the PM-core lock while allowing
	 * BASEPRI-independent wake interrupts to release WFI.
	 */
	key = arch_pm_state_set_prepare();
	__DSB();
	__ISB();
	HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);
	arch_pm_state_set_finish(key);
	LL_LPM_EnableSleep();
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

		uint32_t measured_frequency = HAL_RCC_GetCpuClockFreq();
		uint32_t expected_source = stm32n6_cpu_clock_source();
		uint32_t measured_source = LL_RCC_GetCpuClkSource();

		if (measured_frequency != CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC ||
		    (expected_source != UINT32_MAX &&
		     measured_source != expected_source)) {
			LOG_WRN("clock mismatch: source %u/%u, rate %u/%u",
				measured_source, expected_source,
				measured_frequency,
				CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
		}

		/*
		 * The N6 SoC enables both caches for normal execution. Reassert that
		 * invariant only after RCC restoration; cache management is a no-op when
		 * the corresponding Kconfig option is disabled.
		 */
		sys_cache_instr_enable();
		sys_cache_data_enable();
	}

}
