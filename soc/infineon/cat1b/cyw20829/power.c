/*
 * Copyright (c) 2021 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h>

#include <cyhal_syspm.h>
#include <cyhal_lptimer.h>

LOG_MODULE_REGISTER(soc_power, CONFIG_SOC_LOG_LEVEL);

static uint32_t sleep_attempt_counts;
static uint32_t sleep_counts;
static uint32_t deepsleep_attempt_counts;
static uint32_t deepsleep_counts;
static int64_t last_sleep_start;
static int64_t last_sleep_duration;

void get_sleep_info(uint32_t *sleepattempts, uint32_t *sleepcounts, uint32_t *deepsleepattempts,
		    uint32_t *deepsleepcounts, int64_t *sleeptime)
{
	*sleepattempts = sleep_attempt_counts;
	*sleepcounts = sleep_counts;
	*deepsleepattempts = deepsleep_attempt_counts;
	*deepsleepcounts = deepsleep_counts;
	*sleeptime = last_sleep_duration;
}

/*
 * Called from pm_system_suspend(int32_t ticks) in subsys/power.c
 * For deep sleep pm_system_suspend has executed all the driver
 * power management call backs.
 */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		LOG_DBG("Entering PM state suspend to idle");
		sleep_attempt_counts++;
		last_sleep_start = k_uptime_ticks();
		if (cyhal_syspm_sleep() == CY_RSLT_SUCCESS) {
			sleep_counts++;
		}

		break;
	case PM_STATE_SUSPEND_TO_RAM:
		LOG_DBG("Entering PM state suspend to RAM");
		deepsleep_attempt_counts++;
		last_sleep_start = k_uptime_ticks();
		if (cyhal_syspm_deepsleep() == CY_RSLT_SUCCESS) {
			deepsleep_counts++;
		}

		/*
		 * The HAL function doesn't clear this bit.  It is a problem
		 * if the Zephyr idle function executes the wfi instruction
		 * with this bit set.  We will always clear it here to avoid
		 * that situation.
		 */
		SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
	case PM_STATE_SUSPEND_TO_RAM:
		last_sleep_duration = last_sleep_start - k_uptime_ticks();
		break;

	default:
		break;
	}
}

int ifx_pm_init(void)
{
	/* System Domain Idle Power Mode Configuration */
	Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

	return cyhal_syspm_init();
}
