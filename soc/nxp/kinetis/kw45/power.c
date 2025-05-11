/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/timeout_q.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/timer/system_timer.h>

#include "fsl_pm_core.h"
#include "fsl_pm_board.h"

#include "fwk_platform_lowpower.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define LPTMR0_DEV DT_NODELABEL(lptmr0)
const struct device *const counter_dev = DEVICE_DT_GET(LPTMR0_DEV);

/* TODO: change this line when PowerDown is enabled
 * This definition is needed for compilation, but only used on PowerDown
 */
uint32_t m_warmboot_stack_end __section("RetainedMem");

/* -------------------------------------------------------------------------- */
/*                              Private variables                             */
/* -------------------------------------------------------------------------- */

static pm_handle_t pm_hdl;
static bool unsupported_state;
static uint8_t lowest_state;

/* -------------------------------------------------------------------------- */
/*                              Public functions                              */
/* -------------------------------------------------------------------------- */

__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	int32_t timeout_expiry;
	struct counter_top_cfg counter_info;

	ARG_UNUSED(substate_id);

	__ASSERT(device_is_ready(counter_dev), "ERROR: Counter is not ready to be used");

	__disable_irq();
	irq_unlock(0);

	unsupported_state = false;

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		lowest_state = PM_LP_STATE_DEEP_SLEEP;
		break;

	default:
		LOG_DBG("Unsupported power state %u", state);
		unsupported_state = true;
		break;
	}

	if (!unsupported_state) {

		if (PM_SetConstraints(lowest_state, 0) != 0) {
			__ASSERT(0, "ERROR: to set constraint");
		}

		timeout_expiry = z_get_next_timeout_expiry();

		counter_info.ticks =
			counter_us_to_ticks(counter_dev, k_ticks_to_us_floor32(timeout_expiry));
		counter_info.callback = NULL;
		counter_info.user_data = NULL;

		counter_set_top_value(counter_dev, &counter_info);
		/* Disable systick before going to low power */
		SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);
		/* LPTMR (counter) is set to wakeup the system after the requested time */
		if (counter_start(counter_dev) != 0) {
			__ASSERT(0, "ERROR: can't start timer to wakeup");
		}

		PM_EnterLowPower(k_ticks_to_us_floor64(timeout_expiry));
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	uint32_t slept_time_ticks;
	uint32_t slept_time_us;

	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	if (!unsupported_state) {
		counter_get_value(counter_dev, &slept_time_ticks);
		/* Reactivate systick */
		SysTick->CTRL |= (SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);

		counter_stop(counter_dev);
		slept_time_us = counter_ticks_to_us(counter_dev, slept_time_ticks);
		/* Announce the time slept to the kernel*/
		sys_clock_announce(k_us_to_ticks_near32(slept_time_us));

		if (PM_ReleaseConstraints(lowest_state, 0) != 0) {
			__ASSERT(0, "ERROR: to release constraint");
		}
	}
	/* Clear PRIMASK */
	__enable_irq();
}

static int kw45_power_init(void)
{
	int ret = 0;

	PM_CreateHandle(&pm_hdl);
	PM_EnablePowerManager(true);

	PLATFORM_LowPowerInit();
#if !defined(CONFIG_BT)
	RFMC->CTRL |= RFMC_CTRL_RFMC_RST(0x1U);
	RFMC->CTRL &= ~RFMC_CTRL_RFMC_RST_MASK;

	/* NBU was probably in low power before the RFMC reset, so we need to wait for
	 * the FRO clock to be valid before accessing RF_CMC
	 */
	while ((RFMC->RF2P4GHZ_STAT & RFMC_RF2P4GHZ_STAT_FRO_CLK_VLD_STAT_MASK) == 0U) {
		;
	}

	RF_CMC1->RADIO_LP |= RF_CMC1_RADIO_LP_CK(0x2);

	/* Force low power entry request to the radio domain */
	RFMC->RF2P4GHZ_CTRL |= RFMC_RF2P4GHZ_CTRL_LP_ENTER(0x1U);
#endif
	return ret;
}

SYS_INIT(kw45_power_init, PRE_KERNEL_2, 0);
