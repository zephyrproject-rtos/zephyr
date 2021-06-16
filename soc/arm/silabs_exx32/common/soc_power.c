/*
 * Copyright (c) 2021, Kalyan Kumar
 * Copyright (c) 2018, Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <pm/pm.h>
#include <em_emu.h>

#include <logging/log.h>

#ifdef CONFIG_SOC_GECKO_DEV_INIT

#include "sl_sleeptimer.h"
#include "sl_power_manager.h"
#include <ksched.h>

/* Tick count (in lf ticks) last time the tick count was updated in RTOS. */
static uint32_t last_update_lftick;

/* Expected sleep lf ticks */
static uint32_t expected_sleep_lf_ticks;

/* Total lf ticks slept */
static uint32_t total_slept_lf_ticks;

/* sleep timer frequency */
static uint32_t sleeptimer_freq;

/* timer handler */
static sl_sleeptimer_timer_handle_t schedule_wakeup_timer_handle;

/* indicates scheduling is required or not */
static bool isContextSwitchRequired;

/* idle ticks from idle thread */
static uint32_t IdleTicks;

/* to compensate the os ticks during sleep time */
void sys_clock_announce(int32_t ticks);

/* Local functions */
static void sli_schedule_wakeup_timer_expire_handler(sl_sleeptimer_timer_handle_t *handle,
						     void *data);

static void sli_os_schedule_wakeup(uint32_t expected_idletime_in_os_ticks);

	/* __WEAK is defined to avoid compilation issue for test application
	 * (tests/subsys/power/power_mgmt)
	 */
__WEAK struct pm_state_info pm_policy_next_state(int32_t ticks);


static const struct pm_state_info pm_min_residency[] =
	PM_STATE_INFO_DT_ITEMS_LIST(DT_NODELABEL(cpu0));

#endif

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/*
 * Power state map:
 * PM_STATE_RUNTIME_IDLE: EM1 Sleep
 * PM_STATE_SUSPEND_TO_IDLE: EM2 Deep Sleep
 * PM_STATE_STANDBY: EM3 Stop
 */

/* Invoke Low Power/System Off specific Tasks */
void pm_power_state_set(struct pm_state_info info)
{
	LOG_DBG("SoC entering power state %d", info.state);

#ifdef CONFIG_SOC_GECKO_DEV_INIT

	uint32_t BasePRI;

	/* WorkAround: When this function is entered the Kernel has disabled
	 * interrupts using BASEPRI register. This is incorrect as it prevents
	 * waking up from any interrupt which priority is not 0. Work around the
	 * issue using __set_BASEPRI(0) to remove the BASEPRI masking
	 */

	BasePRI = __get_BASEPRI();
	__set_BASEPRI(0);

	switch (info.state) {

	case PM_STATE_RUNTIME_IDLE:
	case PM_STATE_SUSPEND_TO_IDLE:
	case PM_STATE_STANDBY:

		sleeptimer_freq = sl_sleeptimer_get_timer_frequency();
		__ASSERT(sleeptimer_freq >= CONFIG_SYS_CLOCK_TICKS_PER_SEC,
				"SleepTimer Frequency Not suitable for wakeup Timer");

		last_update_lftick = sl_sleeptimer_get_tick_count();
		/* wake-up the system 1 msec before the actual timeout */
		sli_os_schedule_wakeup(IdleTicks - k_us_to_ticks_ceil32(1000));
		total_slept_lf_ticks = 0;
		sl_power_manager_sleep();
		sl_sleeptimer_stop_timer(&schedule_wakeup_timer_handle);

		if (isContextSwitchRequired == true) {
			isContextSwitchRequired = false;
			/* To invoke the scheduler immediately */
			k_yield();
		}
		break;

	default:
		LOG_DBG("Unsupported power state %u", info.state);
		break;
	}

	LOG_DBG("SoC leaving power state %d", info.state);

	__set_BASEPRI(BasePRI);

#else

	/* FIXME: When this function is entered the Kernel has disabled
	 * interrupts using BASEPRI register. This is incorrect as it prevents
	 * waking up from any interrupt which priority is not 0. Work around the
	 * issue and disable interrupts using PRIMASK register as recommended
	 * by ARM.
	 */

	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (info.state) {
	case PM_STATE_RUNTIME_IDLE:
		EMU_EnterEM1();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		EMU_EnterEM2(true);
		break;
	case PM_STATE_STANDBY:
		EMU_EnterEM3(true);
		break;
	default:
		LOG_DBG("Unsupported power state %u", info.state);
		break;
	}

	LOG_DBG("SoC leaving power state %d", info.state);

	/* Clear PRIMASK */
	__enable_irq();

#endif

}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	ARG_UNUSED(info);
}

#ifdef CONFIG_SOC_GECKO_DEV_INIT

/* Starts a application timer by converting idle ticks to low frequency sleep timer ticks */
static void sli_os_schedule_wakeup(uint32_t expected_idletime_in_os_ticks)
{
	sl_status_t status;
	uint32_t lf_ticks_to_sleep;
	uint32_t current_tick_count = sl_sleeptimer_get_tick_count();

	/* This function implements a correction mechanism that corrects any drift that can */
	/* occur between the sleep timer time and the tick count in RTOS. */

	lf_ticks_to_sleep = (expected_idletime_in_os_ticks * sleeptimer_freq) /
						CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	expected_sleep_lf_ticks = lf_ticks_to_sleep;

	if (lf_ticks_to_sleep <= (current_tick_count - last_update_lftick)) {
		lf_ticks_to_sleep = 1;
	} else {
		lf_ticks_to_sleep -= (current_tick_count - last_update_lftick);
	}

	status = sl_sleeptimer_start_timer(&schedule_wakeup_timer_handle,
					   lf_ticks_to_sleep,
					   sli_schedule_wakeup_timer_expire_handler,
					   0,
					   0,
					   0);
	__ASSERT(status == SL_STATUS_OK, "Failed to create timer");

}

static void sli_schedule_wakeup_timer_expire_handler(sl_sleeptimer_timer_handle_t *handle,
						     void *data)
{
	(void)handle;
	(void)data;

	/* This handler is just to wakeup the device from the low energy mode */
}

/* Function to check any ready threads are scheduled */
bool ConfirmSleepModeStatus(void)
{
	bool ret = true;

	if (!z_is_idle_thread_object(_kernel.ready_q.cache)) {
		/* Issue: sysTick Counter is not incrementing if deep sleep bit is set
		 * WorkAround: Clearing the Deep Sleep Bit , incase not cleared by
		 * sl_power_manager_sleep
		 */
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

		isContextSwitchRequired = true;

		ret = false;
	}
	return ret;
}

/* Function called by power manager to determine if system can go back to sleep
 * after a wakeup
 */
bool sl_power_manager_sleep_on_isr_exit(void)
{
	uint32_t slept_lf_ticks;
	uint32_t slept_os_ticks;

	/* Determine how long we slept. */
	slept_lf_ticks = sl_sleeptimer_get_tick_count() - last_update_lftick;
	slept_os_ticks = (slept_lf_ticks * CONFIG_SYS_CLOCK_TICKS_PER_SEC) / sleeptimer_freq;
	last_update_lftick += slept_lf_ticks;

	/* Notify RTOS of how long we slept. */
	if ((total_slept_lf_ticks + slept_lf_ticks) < expected_sleep_lf_ticks) {
		total_slept_lf_ticks += slept_lf_ticks;
	} else {
		slept_os_ticks = ((expected_sleep_lf_ticks - total_slept_lf_ticks) *
				   CONFIG_SYS_CLOCK_TICKS_PER_SEC) / sleeptimer_freq;
		total_slept_lf_ticks = expected_sleep_lf_ticks;
	}

	if (SCB->SCR & SCB_SCR_SLEEPDEEP_Msk) {
		sys_clock_announce(slept_os_ticks);
	}

	/* Have we slept enough ? */
	if (total_slept_lf_ticks >= expected_sleep_lf_ticks) {
		return false;
	}
	/*  Check if we can sleep again */
	return (ConfirmSleepModeStatus() != false);
}

/* Function called by power manager to determine if system can go to sleep
 * or not
 */
bool sl_power_manager_is_ok_to_sleep(void)
{
	return (ConfirmSleepModeStatus() != false);
}

/* CONFIG_PM_POLICY_APP allows to define application specific power policy
 * checking whether the ticks is meeting the minimum criteria to enter
 * into SOC specific power policy or not
 */
__WEAK struct pm_state_info pm_policy_next_state(int ticks)
{
	int i;

	for (i = ARRAY_SIZE(pm_min_residency) - 1; i >= 0; i--) {
		if (!pm_constraint_get(pm_min_residency[i].state)) {
			continue;
		}

		if ((ticks == K_TICKS_FOREVER) ||
		    (ticks >= k_us_to_ticks_ceil32(
			     pm_min_residency[i].min_residency_us))) {
			LOG_DBG("Selected power state %d "
				"(ticks: %d, min_residency: %u)",
				pm_min_residency[i].state, ticks,
				pm_min_residency[i].min_residency_us);

			/* Busy waiting for 1 tick to flush UART buffer */
			k_busy_wait(k_ticks_to_us_floor32(1));
			IdleTicks = ticks - 1;

			return pm_min_residency[i];
		}
	}
	return (struct pm_state_info){ PM_STATE_ACTIVE, 0, 0 };
}
#endif
