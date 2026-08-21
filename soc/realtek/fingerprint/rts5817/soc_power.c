/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rts5817_clock.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <cmsis_core.h>
#include <zephyr/logging/log.h>

#include "soc_pm_s2ram.h"

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

static void pm_suspend_to_ram(uint32_t substate_id)
{
	soc_s2ram_suspend(substate_id);
}

#ifdef CONFIG_SOC_RTS5817_PM_SUSPEND_IDLE_LOWER_FREQ

#define PM_IDLE_CLK_DEV DEVICE_DT_GET(DT_NODELABEL(clks))

/* CPU/bus frequency in effect before suspend-to-idle, restored on wakeup */
static uint32_t pm_idle_saved_bus_freq;

static void pm_idle_clock_lower(void)
{
	const struct device *clk = PM_IDLE_CLK_DEV;

	pm_idle_saved_bus_freq = 0;

	if (!device_is_ready(clk)) {
		return;
	}

	if (clock_control_get_rate(clk, (clock_control_subsys_t)RTS_FP_CLK_BUS,
				   &pm_idle_saved_bus_freq) != 0) {
		pm_idle_saved_bus_freq = 0;
		return;
	}

	clock_control_set_rate(
		clk, (clock_control_subsys_t)RTS_FP_CLK_BUS,
		(clock_control_subsys_rate_t)CONFIG_SOC_RTS5817_PM_SUSPEND_IDLE_BUS_FREQ);
}

static void pm_idle_clock_restore(void)
{
	if (pm_idle_saved_bus_freq == 0) {
		return;
	}

	clock_control_set_rate(PM_IDLE_CLK_DEV, (clock_control_subsys_t)RTS_FP_CLK_BUS,
			       (clock_control_subsys_rate_t)pm_idle_saved_bus_freq);
}

#else

static inline void pm_idle_clock_lower(void)
{
}

static inline void pm_idle_clock_restore(void)
{
}

#endif /* CONFIG_SOC_RTS5817_PM_SUSPEND_IDLE_LOWER_FREQ */

static void pm_suspend_to_idle(void)
{
	uint32_t key;

	key = arch_pm_state_set_prepare();

	/* Reduce CPU/bus clock while idle to save power */
	pm_idle_clock_lower();

	__DSB();
	__ISB();
	__WFI();

	/* Restore the frequency before the wake ISR runs */
	pm_idle_clock_restore();

	arch_pm_state_set_finish(key);
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	LOG_DBG("PM state: %d, substate_id: %d\n", state, substate_id);

	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
		pm_suspend_to_idle();
		SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
		break;
#ifdef CONFIG_PM_S2RAM
	case PM_STATE_SUSPEND_TO_RAM:
		__disable_irq();
		pm_suspend_to_ram(substate_id);
		__enable_irq();
		break;
#endif
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		break;
#ifdef CONFIG_PM_S2RAM
	case PM_STATE_SUSPEND_TO_RAM:
		break;
#endif
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}
