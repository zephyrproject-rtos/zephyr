/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test.h"

/* In Tickless Kernel mode, time is passed in milliseconds instead of ticks */
#define TICKS_TO_SECONDS_MULTIPLIER 1000
#define TIME_UNIT_STRING "milliseconds"

#define MIN_TIME_TO_SUSPEND	((SECONDS_TO_SLEEP * \
				  TICKS_TO_SECONDS_MULTIPLIER) - \
				  (TICKS_TO_SECONDS_MULTIPLIER / 2))

#ifdef CONFIG_TICKLESS_KERNEL
#define SECS_TO_TICKS		1000
#else
#define SECS_TO_TICKS		100
#endif

#define LPS_MAX_THR		(60 * SECS_TO_TICKS)
#define LPS_1_MAX_THR		(120 * SECS_TO_TICKS)

/* Suspend Devices based on Ordered List */
void suspend_devices(void);
/* Resume Devices */
void resume_devices(void);

static int post_ops_done = 1;
static int pm_state;

/* This Function decides which Low Power Mode to Enter based on Idleness.
 * In actual Application this decision can be made using time (ticks)
 * And RTC timer can be programmed to wake the device after time elapsed.
 */
static int pm_policy_get_state(s32_t ticks)
{
	/*
	 * Power Management Policy:
	 * 0 - 60 Sec:  Enter Constant Latency Mode
	 * 61 - 120 Sec: Enter Low Power state
	 * 121 and above : Enter Deep Sleep state
	 */
	if ((ticks != K_FOREVER) && ticks <= LPS_MAX_THR) {
		return SYS_POWER_STATE_CPU_LPS;
	} else if ((ticks > LPS_MAX_THR) &&
			(ticks <= LPS_1_MAX_THR)) {
		return SYS_POWER_STATE_CPU_LPS_1;
	} else {
		return SYS_POWER_STATE_DEEP_SLEEP;
	}
}

static void low_power_state_exit(void)
{
	/* Perform some Application specific task on Low Power Mode Exit */

	/* Turn on suspended peripherals/clocks as necessary */
	printk("-- Low Power State exit !\n");
}

static int low_power_state_entry(void)
{
	if (pm_state == SYS_POWER_STATE_CPU_LPS) {
		printk("--> Entering into Constant Latency State -");
	} else {
		printk("--> Entering into Low Power State -");
	}
	_sys_soc_set_power_state(pm_state);
	return SYS_PM_LOW_POWER_STATE;
}

static void low_power_suspend_exit(void)
{
	/* Turn on peripherals and restore device states as necessary */
	resume_devices();
	printk("-- Low Power Suspend State exit!\n");
}

static int low_power_suspend_entry(void)
{
	printk("--> Entering into Deep Sleep State -");
	/* Don't need pm idle exit event notification */
	_sys_soc_pm_idle_exit_notification_disable();
	/* Save device states and turn off peripherals as necessary */
	suspend_devices();
	_sys_soc_set_power_state(SYS_POWER_STATE_DEEP_SLEEP);
	 /* Exiting from Deep Sleep State */
	low_power_suspend_exit();

	return SYS_PM_DEEP_SLEEP;
}

/* Function name : _sys_soc_suspend
 * Return Value : Power Mode Entered Success/Failure
 * Input : Idleness in number of Ticks
 *
 * Description: All request from Idle thread to Enter into
 * Low Power Mode or Deep Sleep State land in this function
 */
int _sys_soc_suspend(s32_t ticks)
{
	int ret = SYS_PM_NOT_HANDLED;

	post_ops_done = 0;

	if ((ticks != K_FOREVER) && (ticks < MIN_TIME_TO_SUSPEND)) {
		printk("Not enough time for PM operations " TIME_UNIT_STRING
							" : %d).\n", ticks);
		return SYS_PM_NOT_HANDLED;
	}

	pm_state = pm_policy_get_state(ticks);

	switch (pm_state) {
	case SYS_POWER_STATE_CPU_LPS:
	case SYS_POWER_STATE_CPU_LPS_1:
		/* Do CPU LPS operations */
		ret = low_power_state_entry();
		break;
	case SYS_POWER_STATE_DEEP_SLEEP:
		/* Do Low Power Suspend operations */
		ret = low_power_suspend_entry();
		break;
	default:
		/* No PM operations */
		printk("\nNo PM operations done\n");
		ret = SYS_PM_NOT_HANDLED;
		break;
	}

	if (ret != SYS_PM_NOT_HANDLED) {
		/*
		 * Do any arch or soc specific post operations specific to the
		 * power state.
		 */
		if (!post_ops_done) {
			if ((pm_state == SYS_POWER_STATE_CPU_LPS_1) ||
					(pm_state == SYS_POWER_STATE_CPU_LPS)) {
				low_power_state_exit();
			}
			post_ops_done = 1;
			_sys_soc_power_state_post_ops(pm_state);
		}
	}

	return ret;
}

void _sys_soc_resume(void)
{
	/*
	 * This notification is called from the ISR of the event
	 * that caused exit from kernel idling after PM operations.
	 *
	 * Some CPU low power states require enabling of interrupts
	 * atomically when entering those states. The wake up from
	 * such a state first executes code in the ISR of the interrupt
	 * that caused the wake. This hook will be called from the ISR.
	 * For such CPU LPS states, do post operations and restores here.
	 * The kernel scheduler will get control after the ISR finishes
	 * and it may schedule another thread.
	 *
	 * Call _sys_soc_pm_idle_exit_notification_disable() if this
	 * notification is not required.
	 */
	if (!post_ops_done) {
		if (pm_state == SYS_POWER_STATE_CPU_LPS) {
			low_power_state_exit();
		}
		post_ops_done = 1;
		_sys_soc_power_state_post_ops(pm_state);
	}
}
