/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <init.h>
#include <string.h>
#include <soc.h>
#include "pm_policy.h"

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_REGISTER(power);

static int post_ops_done = 1;
static enum power_states pm_state;

int _sys_soc_suspend(s32_t ticks)
{
	int sys_state;

	post_ops_done = 0;

	sys_state = sys_pm_policy_next_state(ticks, &pm_state);

	switch (sys_state) {
	case SYS_PM_LOW_POWER_STATE:
		sys_pm_notify_lps_entry(pm_state);
		/* Do CPU LPS operations */
		_sys_soc_set_power_state(pm_state);
		break;
	case SYS_PM_DEEP_SLEEP:
		/* Don't need pm idle exit event notification */
		_sys_soc_pm_idle_exit_notification_disable();

		sys_pm_notify_lps_entry(pm_state);

		/* Save device states and turn off peripherals as necessary */
		if (sys_pm_suspend_devices()) {
			LOG_ERR("System level device suspend failed\n");
			break;
		}

		/* Enter CPU deep sleep state */
		_sys_soc_set_power_state(pm_state);

		/* Turn on peripherals and restore device states as necessary */
		sys_pm_resume_devices();
		break;
	default:
		/* No PM operations */
		LOG_DBG("\nNo PM operations done\n");
		break;
	}

	if (sys_state != SYS_PM_NOT_HANDLED) {
		/*
		 * Do any arch or soc specific post operations specific to the
		 * power state.
		 */
		if (!post_ops_done) {
			post_ops_done = 1;
			sys_pm_notify_lps_exit(pm_state);
			_sys_soc_power_state_post_ops(pm_state);
		}
	}

	return sys_state;
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
		post_ops_done = 1;
		sys_pm_notify_lps_exit(pm_state);
		_sys_soc_power_state_post_ops(pm_state);
	}
}

static int sys_pm_init(struct device *dev)
{
	ARG_UNUSED(dev);

	sys_pm_create_device_list();
	return 0;
}

SYS_INIT(sys_pm_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
