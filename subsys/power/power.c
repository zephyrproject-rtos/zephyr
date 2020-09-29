/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <init.h>
#include <string.h>
#include <power/power.h>
#include "policy/pm_policy.h"

#define LOG_LEVEL CONFIG_SYS_PM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(power);

static int post_ops_done = 1;
static enum power_states forced_pm_state = SYS_POWER_STATE_AUTO;
static enum power_states pm_state;

#ifdef CONFIG_SYS_PM_DEBUG

struct pm_debug_info {
	uint32_t count;
	uint32_t last_res;
	uint32_t total_res;
};

static struct pm_debug_info pm_dbg_info[SYS_POWER_STATE_MAX];
static uint32_t timer_start, timer_end;

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
	uint32_t res = timer_end - timer_start;

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

__weak void sys_pm_notify_power_state_entry(enum power_states state)
{
	/* This function can be overridden by the application. */
}

__weak void sys_pm_notify_power_state_exit(enum power_states state)
{
	/* This function can be overridden by the application. */
}

void sys_pm_force_power_state(enum power_states state)
{
	__ASSERT(state >= SYS_POWER_STATE_AUTO &&
		 state <  SYS_POWER_STATE_MAX,
		 "Invalid power state %d!", state);

#ifdef CONFIG_SYS_PM_DIRECT_FORCE_MODE
	(void)arch_irq_lock();
	forced_pm_state = state;
	_sys_suspend(K_TICKS_FOREVER);
#else
	forced_pm_state = state;
#endif
}

enum power_states _sys_suspend(int32_t ticks)
{
	bool deep_sleep;
#if CONFIG_DEVICE_POWER_MANAGEMENT
	bool low_power = false;
#endif

	pm_state = (forced_pm_state == SYS_POWER_STATE_AUTO) ?
		   sys_pm_policy_next_state(ticks) : forced_pm_state;

	if (pm_state == SYS_POWER_STATE_ACTIVE) {
		LOG_DBG("No PM operations done.");
		return pm_state;
	}

	deep_sleep = IS_ENABLED(CONFIG_SYS_POWER_DEEP_SLEEP_STATES) ?
		     sys_pm_is_deep_sleep_state(pm_state) : 0;

	post_ops_done = 0;
	sys_pm_notify_power_state_entry(pm_state);

	if (deep_sleep) {
#if CONFIG_DEVICE_POWER_MANAGEMENT
		/* Suspend peripherals. */
		if (sys_pm_suspend_devices()) {
			LOG_DBG("Some devices didn't enter suspend state!");
			sys_pm_resume_devices();
			sys_pm_notify_power_state_exit(pm_state);
			pm_state = SYS_POWER_STATE_ACTIVE;
			return pm_state;
		}
#endif
		/*
		 * Disable idle exit notification as it is not needed
		 * in deep sleep mode.
		 */
		_sys_pm_idle_exit_notification_disable();
#if CONFIG_DEVICE_POWER_MANAGEMENT
	} else {
		if (sys_pm_policy_low_power_devices(pm_state)) {
			/* low power peripherals. */
			if (sys_pm_low_power_devices()) {
				LOG_DBG("Someone didn't enter low power state");
				sys_pm_resume_devices();
				sys_pm_notify_power_state_exit(pm_state);
				pm_state = SYS_POWER_STATE_ACTIVE;
				return pm_state;
			}

			low_power = true;
		}
#endif
	}

	/* Enter power state */
	sys_pm_debug_start_timer();
	sys_set_power_state(pm_state);
	sys_pm_debug_stop_timer();

#if CONFIG_DEVICE_POWER_MANAGEMENT
	if (deep_sleep || low_power) {
		/* Turn on peripherals and restore device states as necessary */
		sys_pm_resume_devices();
	}
#endif
	sys_pm_log_debug_info(pm_state);

	if (!post_ops_done) {
		post_ops_done = 1;
		/* clear forced_pm_state */
		forced_pm_state = SYS_POWER_STATE_AUTO;
		sys_pm_notify_power_state_exit(pm_state);
		_sys_pm_power_state_exit_post_ops(pm_state);
	}

	return pm_state;
}

void _sys_resume(void)
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
	 * Call _sys_pm_idle_exit_notification_disable() if this
	 * notification is not required.
	 */
	if (!post_ops_done) {
		post_ops_done = 1;
		sys_pm_notify_power_state_exit(pm_state);
		_sys_pm_power_state_exit_post_ops(pm_state);
	}
}

#if CONFIG_DEVICE_POWER_MANAGEMENT
static int sys_pm_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	sys_pm_create_device_list();
	return 0;
}

SYS_INIT(sys_pm_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */
