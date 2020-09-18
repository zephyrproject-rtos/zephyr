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
#include <power/power_state.h>

#define LOG_LEVEL CONFIG_SYS_PM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(power);

static int post_ops_done = 1;
static pm_state_t forced_pm_state;
static pm_state_t pm_state;

#ifdef CONFIG_SYS_PM_DEBUG

struct pm_debug_info {
	uint32_t count;
	uint32_t last_res;
	uint32_t total_res;
};

static struct pm_debug_info pm_dbg_info[PM_STATE_COUNT];
static uint32_t timer_start, timer_end;

static inline void sys_pm_debug_start_timer(void)
{
	timer_start = k_cycle_get_32();
}

static inline void sys_pm_debug_stop_timer(void)
{
	timer_end = k_cycle_get_32();
}

static void sys_pm_log_debug_info(pm_state_t state)
{
	uint32_t res = timer_end - timer_start;

	for (int i = 0; i < PM_STATE_COUNT; i++) {
		if (state == BIT(i)) {
			pm_dbg_info[i].count++;
			pm_dbg_info[i].last_res = res;
			pm_dbg_info[i].total_res += res;

			break;
		}
	}
}

void sys_pm_dump_debug_info(void)
{
	for (int i = 0; i < PM_STATE_COUNT; i++) {
		LOG_DBG("PM:state = %ld, count = %d last_res = %d, "
			"total_res = %d\n", BIT(i), pm_dbg_info[i].count,
			pm_dbg_info[i].last_res, pm_dbg_info[i].total_res);
	}
}
#else
static inline void sys_pm_debug_start_timer(void) { }
static inline void sys_pm_debug_stop_timer(void) { }
static void sys_pm_log_debug_info(pm_state_t state) { }
void sys_pm_dump_debug_info(void) { }
#endif

__weak void sys_pm_notify_power_state_entry(pm_state_t state)
{
	/* This function can be overridden by the application. */
}

__weak void sys_pm_notify_power_state_exit(pm_state_t state)
{
	/* This function can be overridden by the application. */
}

void sys_pm_force_power_state(pm_state_t state)
{
#ifdef CONFIG_SYS_PM_DIRECT_FORCE_MODE
	(void)arch_irq_lock();
	forced_pm_state = state;
	_sys_suspend(K_TICKS_FOREVER);
#else
	forced_pm_state = state;
#endif
}

pm_state_t _sys_suspend(int32_t ticks)
{
#if CONFIG_DEVICE_POWER_MANAGEMENT
	dev_pm_state_t dev_pm_state;
#endif

	pm_state = sys_pm_policy_next_state(ticks);

	if (pm_state == PM_STATE_RUNTIME_ACTIVE) {
		LOG_DBG("No PM operations done.");
		return pm_state;
	}

	post_ops_done = 0;

	sys_pm_notify_power_state_entry(pm_state);

#if CONFIG_DEVICE_POWER_MANAGEMENT
	dev_pm_state = pm_state_sys2dev(pm_state);

	if (dev_pm_state == DEVICE_PM_SUSPEND_STATE) {
		if (sys_pm_low_power_devices()) {
			LOG_DBG("Someone didn't enter low power state");
			sys_pm_resume_devices();
			sys_pm_notify_power_state_exit(pm_state);
			pm_state = PM_STATE_RUNTIME_ACTIVE;
			return pm_state;
		}
	} else if (dev_pm_state == DEVICE_PM_OFF_STATE) {
		if (sys_pm_suspend_devices()) {
			LOG_DBG("Some devices didn't enter suspend state!");
			sys_pm_resume_devices();
			sys_pm_notify_power_state_exit(pm_state);
			pm_state = PM_STATE_RUNTIME_ACTIVE;
			return pm_state;
		}
	}
#endif

	/* Enter power state */
	sys_pm_debug_start_timer();
	sys_set_power_state(pm_state);
	sys_pm_debug_stop_timer();

#if CONFIG_DEVICE_POWER_MANAGEMENT
	if (dev_pm_state == DEVICE_PM_SUSPEND_STATE ||
	    dev_pm_state == DEVICE_PM_OFF_STATE) {
		/* Turn on peripherals and restore device states as necessary */
		sys_pm_resume_devices();
	}
#endif
	sys_pm_log_debug_info(pm_state);

	if (!post_ops_done) {
		post_ops_done = 1;
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
static int sys_pm_init(struct device *dev)
{
	ARG_UNUSED(dev);

	sys_pm_create_device_list();
	return 0;
}

SYS_INIT(sys_pm_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */
