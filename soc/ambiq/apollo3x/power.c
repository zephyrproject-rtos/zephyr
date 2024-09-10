/*
 * Copyright (c) 2024 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/init.h>

/* ambiq-sdk includes */
#include <am_mcu_apollo.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	__disable_irq();
	__set_BASEPRI(0);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		/* Put ARM core to normal sleep. */
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_NORMAL);
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		/* Put ARM core to deep sleep. */
		/* Cotex-m: power down, register value preserve.*/
		/* Cache: power down*/
		/* Flash: power down*/
		/* Sram: retention*/
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/**
 * @brief PM State Exit Post Operations
 *
 * For PM_STATE_SUSPEND_TO_IDLE:
 *   Nothing is needed after soc woken up.
 *
 * For PM_STATE_SUSPEND_TO_RAM:
 *   Flash, cache, sram automatically switch
 *   to active state on wake up
 *
 * @param state PM State
 * @param substate_id Unused
 *
 */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	__enable_irq();
	irq_unlock(0);
}

void ambiq_power_init(void)
{
	/* Enable flash.
	 * Currently all flash area is powered on, but we should only enable the used flash area and
	 * put unused flash in power down mode.
	 */
	if (am_hal_pwrctrl_memory_enable(AM_HAL_PWRCTRL_MEM_FLASH_MAX)) {
		__ASSERT(0, "Failed to enable FLASH!");
	}

	/* Enable SRAM.
	 * Currently all SRAM area is powered on, but we should only enable the used ram area and
	 * put unused ram in power down mode.
	 */
	if (am_hal_pwrctrl_memory_enable(AM_HAL_PWRCTRL_MEM_SRAM_MAX)) {
		__ASSERT(0, "Failed to enable SRAM!");
	}

	/* For optimal Deep Sleep current, configure cache to be powered-down in deepsleep. */
	am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_CACHE);

	/* Power off all flash area, when go to deep sleep.*/
	am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_FLASH_MAX);

	/* Keep the used SRAM area in retention mode. */
	am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_SRAM_MAX);
	am_hal_pwrctrl_memory_deepsleep_retain(AM_HAL_PWRCTRL_MEM_SRAM_MAX);

#if defined(CONFIG_SOC_APOLLO3P_BLUE)
	/*
	 * If user has enabled AM_HAL_SYSCTRL_DEEPSLEEP_WA in am_hal_sysctrl.h
	 * this will allow user to acheive lower current draw in deepsleep
	 */
	am_hal_sysctrl_control(AM_HAL_SYSCTRL_CONTROL_DEEPSLEEP_MINPWR_EN, 0);
#endif
}
