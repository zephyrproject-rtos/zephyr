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
#include "policy/pm_policy.h"

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(power);

static int post_ops_done = 1;
static enum power_states pm_state;

#ifdef CONFIG_PM_CONTROL_OS_DEBUG

struct pm_debug_info {
	u32_t count;
	u32_t last_res;
	u32_t total_res;
};

static struct pm_debug_info pm_dbg_info[SYS_POWER_STATE_MAX];
static u32_t timer_start, timer_end;

static inline void sys_pm_debug_start_timer(void)
{
	timer_start = k_cycle_get_32();
}

static inline void sys_pm_debug_stop_timer(void)
{
	timer_end = k_cycle_get_32();
}

static void sys_pm_log_debug_info(enum power_states state)
{
	u32_t res = timer_end - timer_start;

	pm_dbg_info[state].count++;
	pm_dbg_info[state].last_res = res;
	pm_dbg_info[state].total_res += res;
}

void sys_pm_dump_debug_info(void)
{
	for (int i = 0; i < SYS_POWER_STATE_MAX; i++) {
		LOG_DBG("PM:state = %d, count = %d last_res = %d, "
			"total_res = %d\n", i, pm_dbg_info[i].count,
			pm_dbg_info[i].last_res, pm_dbg_info[i].total_res);
	}
}
#else
static inline void sys_pm_debug_start_timer(void) { }
static inline void sys_pm_debug_stop_timer(void) { }
static void sys_pm_log_debug_info(enum power_states state) { }
void sys_pm_dump_debug_info(void) { }
#endif

int _sys_soc_suspend(s32_t ticks)
{
	int sys_state;

	post_ops_done = 0;

	sys_state = sys_pm_policy_next_state(ticks, &pm_state);

#ifdef CONFIG_PM_CONTROL_STATE_LOCK
	/* Check if PM state is locked */
	if ((sys_state != SYS_PM_NOT_HANDLED) &&
			!sys_pm_ctrl_is_state_enabled(sys_state)) {
		LOG_DBG("PM state locked %d\n", sys_state);
		return SYS_PM_NOT_HANDLED;
	}
#endif

	switch (sys_state) {
	case SYS_PM_LOW_POWER_STATE:
		sys_pm_notify_lps_entry(pm_state);

		/* Do CPU LPS operations */
		sys_pm_debug_start_timer();
		_sys_soc_set_power_state(pm_state);
		sys_pm_debug_stop_timer();
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
		sys_pm_debug_start_timer();
		_sys_soc_set_power_state(pm_state);
		sys_pm_debug_stop_timer();

		/* Turn on peripherals and restore device states as necessary */
		sys_pm_resume_devices();
		break;
	default:
		/* No PM operations */
		LOG_DBG("\nNo PM operations done\n");
		break;
	}

	if (sys_state != SYS_PM_NOT_HANDLED) {

		sys_pm_log_debug_info(pm_state);
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
