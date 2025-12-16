/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h>

#include <cy_syspm.h>

#include <soc.h>

LOG_MODULE_REGISTER(soc_power, CONFIG_SOC_LOG_LEVEL);

uint32_t sleep_attempt_counts;
uint32_t sleep_counts;
uint32_t deepsleep_attempt_counts;
uint32_t deepsleep_counts;
int64_t last_sleep_start;
int64_t last_sleep_duration;

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
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Switch to using PRIMASK instead of BASEPRI register, since
	 * we are only able to wake up from standby while using PRIMASK.
	 */
	/* Set PRIMASK */
	__disable_irq();

	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		LOG_DBG("Entering PM state suspend to idle");
		sleep_attempt_counts++;
		last_sleep_start = k_uptime_ticks();
		if (Cy_SysPm_CpuEnterSleep(CY_SYSPM_WAIT_FOR_INTERRUPT) == CY_RSLT_SUCCESS) {
			sleep_counts++;
		}
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		LOG_DBG("Entering PM state suspend to RAM");
		deepsleep_attempt_counts++;
		last_sleep_start = k_uptime_ticks();
		if (Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT) == CY_RSLT_SUCCESS) {
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

/*
 * Zephyr PM code expects us to enabled interrupts at post op exit. Zephyr used
 * arch_irq_lock() which sets BASEPRI to a non-zero value masking all interrupts
 * preventing wake. MCHP z_power_soc_(deep)_sleep sets PRIMASK=1 and BASEPRI=0
 * allowing wake from any enabled interrupt and prevents the CPU from entering
 * an ISR on wake except for faults. We re-enable interrupts by setting PRIMASK
 * to 0.
 */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Clear PRIMASK */
	__enable_irq();

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
	case PM_STATE_SUSPEND_TO_RAM:
		last_sleep_duration = last_sleep_start - k_uptime_ticks();
		break;

	default:
		break;
	}
}

static int ifx_pm_init(void)
{
	Cy_SysPm_Init();
	Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

	return 0;
}

SYS_INIT(ifx_pm_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
