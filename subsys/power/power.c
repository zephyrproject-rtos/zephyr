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

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(power);

static int post_ops_done = 1;
static enum power_states forced_pm_state = POWER_STATE_AUTO;
static enum power_states pm_state;

#ifdef CONFIG_PM_DEBUG

struct pm_debug_info {
	uint32_t count;
	uint32_t last_res;
	uint32_t total_res;
};

static struct pm_debug_info pm_dbg_info[POWER_STATE_MAX];
static uint32_t timer_start, timer_end;

static inline void pm_debug_start_timer(void)
{
	timer_start = k_cycle_get_32();
}

static inline void pm_debug_stop_timer(void)
{
	timer_end = k_cycle_get_32();
}

static void pm_log_debug_info(enum power_states state)
{
	uint32_t res = timer_end - timer_start;

	pm_dbg_info[state].count++;
	pm_dbg_info[state].last_res = res;
	pm_dbg_info[state].total_res += res;
}

void pm_dump_debug_info(void)
{
	for (int i = 0; i < POWER_STATE_MAX; i++) {
		LOG_DBG("PM:state = %d, count = %d last_res = %d, "
			"total_res = %d\n", i, pm_dbg_info[i].count,
			pm_dbg_info[i].last_res, pm_dbg_info[i].total_res);
	}
}
#else
static inline void pm_debug_start_timer(void) { }
static inline void pm_debug_stop_timer(void) { }
static void pm_log_debug_info(enum power_states state) { }
void pm_dump_debug_info(void) { }
#endif

__weak void pm_notify_power_state_entry(enum power_states state)
{
	/* This function can be overridden by the application. */
}

__weak void pm_notify_power_state_exit(enum power_states state)
{
	/* This function can be overridden by the application. */
}

void pm_power_state_force(enum power_states state)
{
	__ASSERT(state >= POWER_STATE_AUTO &&
		 state <  POWER_STATE_MAX,
		 "Invalid power state %d!", state);

#ifdef CONFIG_PM_DIRECT_FORCE_MODE
	(void)arch_irq_lock();
	forced_pm_state = state;
	pm_system_suspend(K_TICKS_FOREVER);
#else
	forced_pm_state = state;
#endif
}


static enum power_states _handle_device_abort(enum power_states state)
{
	LOG_DBG("Some devices didn't enter suspend state!");
	pm_resume_devices();
	pm_notify_power_state_exit(pm_state);
	pm_state = POWER_STATE_ACTIVE;
	return pm_state;
}

enum power_states pm_policy_mgr(int32_t ticks)
{
	bool deep_sleep;
#if CONFIG_PM_DEVICE
	bool low_power = false;
#endif

	pm_state = (forced_pm_state == POWER_STATE_AUTO) ?
		   pm_policy_next_state(ticks) : forced_pm_state;

	if (pm_state == POWER_STATE_ACTIVE) {
		LOG_DBG("No PM operations to be done.");
		return pm_state;
	}

	deep_sleep = IS_ENABLED(CONFIG_PM_DEEP_SLEEP_STATES) ?
		     pm_is_deep_sleep_state(pm_state) : 0;

	post_ops_done = 0;
	pm_notify_power_state_entry(pm_state);

	if (deep_sleep) {
		/* Suspend peripherals. */
		if (IS_ENABLED(CONFIG_PM_DEVICE) && pm_suspend_devices()) {
			return _handle_device_abort(pm_state);
		}
		/*
		 * Disable idle exit notification as it is not needed
		 * in deep sleep mode.
		 */
		pm_idle_exit_notification_disable();
#if CONFIG_PM_DEVICE
	} else {
		if (pm_policy_low_power_devices(pm_state)) {
			/* low power peripherals. */
			if (pm_low_power_devices()) {
				return _handle_device_abort(pm_state);
			}

			low_power = true;
		}
#endif
	}

	pm_debug_start_timer();
	/* Enter power state */
	pm_power_state_set(pm_state);
	pm_debug_stop_timer();

	/* Wake up sequence starts here */
#if CONFIG_PM_DEVICE
	if (deep_sleep || low_power) {
		/* Turn on peripherals and restore device states as necessary */
		pm_resume_devices();
	}
#endif
	pm_log_debug_info(pm_state);

	if (!post_ops_done) {
		post_ops_done = 1;
		/* clear forced_pm_state */
		forced_pm_state = POWER_STATE_AUTO;
		pm_notify_power_state_exit(pm_state);
		_pm_power_state_exit_post_ops(pm_state);
	}

	return pm_state;
}


enum power_states pm_system_suspend(int32_t ticks)
{
	return pm_policy_mgr(ticks);
}

void pm_system_resume(void)
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
	 * Call pm_idle_exit_notification_disable() if this
	 * notification is not required.
	 */
	if (!post_ops_done) {
		post_ops_done = 1;
		pm_notify_power_state_exit(pm_state);
		_pm_power_state_exit_post_ops(pm_state);
	}
}

#if CONFIG_PM_DEVICE
static int pm_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	pm_create_device_list();
	return 0;
}

SYS_INIT(pm_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_PM_DEVICE */
