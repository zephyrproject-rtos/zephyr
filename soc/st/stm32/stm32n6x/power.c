/*
 * Copyright (c) 2026 Hula Earth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <clock_control/clock_stm32_ll_common.h>
#include <soc.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <stm32_ll_cortex.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
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
BUILD_ASSERT(DT_NODE_HAS_STATUS(SYSTEM_TIMER_COMPANION_NODE, okay),
	     "STM32N6x power management needs the RTC enabled");

static void stm32n6_prepare_stop(void)
{
	/*
	 * The RTC companion schedules normal PM wakeups; enabled EXTI interrupts are
	 * independent asynchronous wake sources.
	 */
	LL_LPM_DisableEventOnPend();
	LL_LPM_DisableSleepOnExit();
	LL_PWR_ClearFlag_STOP_SB();
	__HAL_RCC_PWR_CLK_ENABLE();
	LL_PWR_EnableBkUpAccess();

	/* The BSEC clock must remain enabled for the CPU deep-sleep request. */
	__HAL_RCC_BSEC_CLK_ENABLE();
	__HAL_RCC_BSEC_CLK_SLEEP_ENABLE();

	/* Keep both RTC clock domains available while the CPU is stopped. */
	__HAL_RCC_RTC_ENABLE();
	__HAL_RCC_RTCAPB_CLK_ENABLE();
	__HAL_RCC_RTC_CLK_SLEEP_ENABLE();
	__HAL_RCC_RTCAPB_CLK_SLEEP_ENABLE();
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	unsigned int key;

	if (state != PM_STATE_SUSPEND_TO_IDLE || substate_id != 1U) {
		LOG_DBG("Unsupported power state %u substate-id %u", state, substate_id);
		return;
	}

	stm32n6_prepare_stop();
	LL_PWR_SetPowerDownModeDS(LL_PWR_POWERDOWN_MODE_DS_STOP);
	LL_LPM_EnableDeepSleep();
	key = arch_pm_state_set_prepare();
	__DSB();
	__ISB();
	__WFI();
	arch_pm_state_set_finish(key);
	LL_LPM_EnableSleep();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	int ret;

	if (state == PM_STATE_SUSPEND_TO_IDLE && substate_id == 1U) {
		LL_LPM_DisableSleepOnExit();
		LL_LPM_EnableSleep();

		/*
		 * STM32N6 STOP returns with the app clock tree unavailable. Restore the
		 * configured safe run point before the kernel timer exit hook or the wake
		 * ISR can execute normal application code.
		 */
		stm32n6_configure_run_power();
		ret = stm32_clock_control_init(NULL);

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
}
