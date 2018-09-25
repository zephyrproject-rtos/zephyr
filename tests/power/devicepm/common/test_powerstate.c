/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_power
 * @{
 * @defgroup t_devicepm test_devicepm
 * @brief TestPurpose: verify device suspend/resume across deep sleep,
 * low power state, and tickless idle. See also @ref driver
 * @details
 * This test case suspend uart and system devices before entering deep sleep,
 * then resume those devices after waking up from deep sleep, UART
 * device supposed to keep output via console.
 * This test case also provide hook APIs for other test cases usage to verify
 * device suspend/resume across deep sleep state. See below hook_dev_xxx().
 * - TestSteps
 *   -# wait semaphore main(), trigger kernel idle loop _sys_soc_suspend()
 *   -# config wakeup event
 *   -# suspend uart and system devices (sysclock, ioapic, loapic)
 *   -# suspend other devices <b>hook_dev_state()</b>
 *   -# enter deep sleep state
 *   -# wakeup from rtc
 *   -# resume system devices and uart
 *   -# resume other devices <b>hook_dev_state()</b>
 *   -# signal semaphore in main()
 *   -# test functionality of resumed devices <b>hook_dev_func()</b>
 *   - ExpectedResults
 *   -# This test case itself does not verify SOC in deep sleep state
 *   -# Verify uart console device resumed functionality as expected
 *   - VerifiedPlatfomrs
 *   -# quark_se_c1000_devboard
 *   @}
 */

#include <ztest.h>
#include <power.h>
#include <soc_power.h>
#include <counter.h>
#include <rtc.h>

#define DURATION (RTC_ALARM_SECOND*2)
#define SLEEP_MS 200
#define STATE_INVALID  4

extern void test_sysdev_state(int state);
/*Hook API to set device power state*/
void (*hook_dev_state)(int) = NULL;
/*Hook API for other test cases to verify functionality on resumed device.*/
void (*hook_dev_func)(void) = NULL;
int rtc_wakeup;

static struct k_sem sync;
static int pmstate = STATE_INVALID;

static struct device *aon_dev, *rtc_dev;
static void wakeup_config(uint32_t value)
{
	aon_dev = device_get_binding(CONFIG_AON_TIMER_QMSI_DEV_NAME);
	rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
	/*when test aon suspend/resume, config wakeup from rtc timer*/
	if (rtc_wakeup) {
		struct rtc_config config;

		config.init_val = 0;
		config.alarm_enable = 1;
		config.alarm_val = value;
		config.cb_fn = NULL;
		rtc_enable(rtc_dev);
		rtc_set_config(rtc_dev, &config);
	} else {
		zassert_not_null(aon_dev, NULL);
		zassert_false(counter_start(aon_dev), NULL);
		zassert_false(counter_set_alarm(aon_dev, NULL, value, NULL),
			      NULL);
	}
}

static struct sys_power_state {
	int device_state;
	int soc_state;
	char name[30];
} sys_state[] = {
	{DEVICE_PM_ACTIVE_STATE, SYS_POWER_STATE_MAX, "SYS_PM_ACTIVE_STATE"},
	{DEVICE_PM_SUSPEND_STATE, SYS_POWER_STATE_CPU_LPS, "SYS_PM_LOW_POWER_STATE"},
	{DEVICE_PM_SUSPEND_STATE, SYS_POWER_STATE_DEEP_SLEEP, "SYS_PM_DEEP_SLEEP"},
};

void exit_sys_power_state(int index)
{
	if (sys_state[index].soc_state != SYS_POWER_STATE_MAX) {
		counter_stop(aon_dev);
	}

	/*resume system devices*/
	test_sysdev_state(DEVICE_PM_ACTIVE_STATE);

	/*resume external devices*/
	if (hook_dev_state) {
		hook_dev_state(DEVICE_PM_ACTIVE_STATE);
	}
}

static void enter_sys_power_state(int index)
{
	/*suspend external devices*/
	if (hook_dev_state) {
		hook_dev_state(sys_state[index].device_state);
	}

	/*suspend system devices*/
	test_sysdev_state(sys_state[index].device_state);

	/*enter soc power state*/
	if (sys_state[index].soc_state != SYS_POWER_STATE_MAX) {
		sys_pm_idle_exit_notification_disable();
		wakeup_config(DURATION);
		TC_PRINT("wakeup configured\n");
		sys_set_power_state(sys_state[index].soc_state);
		/* exit, assume CPU contexts are recovered*/
		exit_sys_power_state(index);
	}
}

/**
 * @brief Hook API invoked by kernel before entering suspend
 * @param ticks Duration in ticks kernel about to be idle
 * @return Non-zero tells kernel not to enter idle state.
 * Zero tells kernel proceed to enter idle state.
 */
int _sys_soc_suspend(int32_t ticks)
{
	switch (pmstate) {
	case SYS_PM_LOW_POWER_STATE:
		TC_PRINT("enter SYS_PM_LOW_POWER_STATE\n");
		enter_sys_power_state(SYS_PM_LOW_POWER_STATE);
		TC_PRINT("exit SYS_PM_LOW_POWER_STATE\n");
		pmstate = SYS_PM_ACTIVE_STATE;
		k_sem_give(&sync);
		return SYS_PM_LOW_POWER_STATE;
	case SYS_PM_DEEP_SLEEP:
		TC_PRINT("enter SYS_PM_DEEP_SLEEP\n");
		enter_sys_power_state(SYS_PM_DEEP_SLEEP);
		TC_PRINT("exit SYS_PM_DEEP_SLEEP\n");
		pmstate = SYS_PM_ACTIVE_STATE;
		k_sem_give(&sync);
		return SYS_PM_DEEP_SLEEP;
	default:
		return SYS_PM_NOT_HANDLED;
	}
}

/**
 * @brief Hook API invoked by kenrel when exiting from low power
 * @return N/A
 */
void _sys_soc_resume(void)
{
}

/**
 * @brief power state transition active->deepsleep->wakeup
 * @return N/A
 */
void test_deepsleep(void)
{
	k_sem_init(&sync, 0, 1);
	pmstate = SYS_PM_DEEP_SLEEP;
	k_sem_take(&sync, K_FOREVER);
	/*test functionality of resumed device*/
	if (hook_dev_func) {
		hook_dev_func();
	}
}

/**
 * @brief power state transition active->lowpower->active
 * @return N/A
 */
void test_lowpower(void)
{
	k_sem_init(&sync, 0, 1);
	pmstate = SYS_PM_LOW_POWER_STATE;
	k_sem_take(&sync, K_FOREVER);
	/*test functionality of resumed device*/
	if (hook_dev_func) {
		hook_dev_func();
	}
}
