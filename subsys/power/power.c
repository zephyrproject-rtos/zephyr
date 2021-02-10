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
#include <power/power_state.h>
#include "policy/pm_policy.h"

#define PM_STATES_LEN (1 + PM_STATE_SOFT_OFF - PM_STATE_ACTIVE)
#define LOG_LEVEL CONFIG_PM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(power);

static int post_ops_done = 1;
static bool z_forced_power_state;
static struct pm_state_info z_power_state;
static sys_slist_t pm_notifiers = SYS_SLIST_STATIC_INIT(&pm_notifiers);
static struct k_spinlock pm_notifier_lock;

#ifdef CONFIG_PM_DEBUG

struct pm_debug_info {
	uint32_t count;
	uint32_t last_res;
	uint32_t total_res;
};

static struct pm_debug_info pm_dbg_info[PM_STATES_LEN];
static uint32_t timer_start, timer_end;

static inline void pm_debug_start_timer(void)
{
	timer_start = k_cycle_get_32();
}

static inline void pm_debug_stop_timer(void)
{
	timer_end = k_cycle_get_32();
}

static void pm_log_debug_info(enum pm_state state)
{
	uint32_t res = timer_end - timer_start;

	pm_dbg_info[state].count++;
	pm_dbg_info[state].last_res = res;
	pm_dbg_info[state].total_res += res;
}

void pm_dump_debug_info(void)
{
	for (int i = 0; i < PM_STATES_LEN; i++) {
		LOG_DBG("PM:state = %d, count = %d last_res = %d, "
			"total_res = %d\n", i, pm_dbg_info[i].count,
			pm_dbg_info[i].last_res, pm_dbg_info[i].total_res);
	}
}
#else
static inline void pm_debug_start_timer(void) { }
static inline void pm_debug_stop_timer(void) { }
static void pm_log_debug_info(enum pm_state state) { }
void pm_dump_debug_info(void) { }
#endif

__weak void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	/*
	 * This function is supposed to be overridden to do SoC or
	 * architecture specific post ops after sleep state exits.
	 */
}

__weak void pm_power_state_set(struct pm_state_info info)
{
	/*
	 * This function is supposed to be overridden to do SoC or
	 * architecture specific post ops after sleep state exits.
	 */
}

void pm_power_state_force(struct pm_state_info info)
{
	__ASSERT(info.state < PM_STATES_LEN,
		 "Invalid power state %d!", info.state);

#ifdef CONFIG_PM_DIRECT_FORCE_MODE
	(void)arch_irq_lock();
	z_forced_power_state = true;
	z_power_state = info;
	pm_system_suspend(K_TICKS_FOREVER);
#else
	z_power_state = info;
	z_forced_power_state = true;
#endif
}

/*
 * Function called to notify when the system is entering / exiting a
 * power state
 */
static inline void pm_state_notify(bool entering_state)
{
	struct pm_notifier *notifier;
	k_spinlock_key_t pm_notifier_key;
	void (*callback)(enum pm_state state);

	pm_notifier_key = k_spin_lock(&pm_notifier_lock);
	SYS_SLIST_FOR_EACH_CONTAINER(&pm_notifiers, notifier, _node) {
		if (entering_state) {
			callback = notifier->state_entry;
		} else {
			callback = notifier->state_exit;
		}

		if (callback) {
			callback(z_power_state.state);
		}
	}
	k_spin_unlock(&pm_notifier_lock, pm_notifier_key);
}

static enum pm_state _handle_device_abort(struct pm_state_info info)
{
	LOG_DBG("Some devices didn't enter suspend state!");
	pm_resume_devices();
	pm_state_notify(false);

	z_power_state.state = PM_STATE_ACTIVE;
	return PM_STATE_ACTIVE;
}

static enum pm_state pm_policy_mgr(int32_t ticks)
{
	bool deep_sleep;
#if CONFIG_PM_DEVICE
	bool low_power = false;
#endif

	if (z_forced_power_state == false) {
		z_power_state = pm_policy_next_state(ticks);
	}

	if (z_power_state.state == PM_STATE_ACTIVE) {
		LOG_DBG("No PM operations done.");
		return z_power_state.state;
	}

	deep_sleep = pm_is_deep_sleep_state(z_power_state.state);

	post_ops_done = 0;
	pm_state_notify(true);

	if (deep_sleep) {
		/* Suspend peripherals. */
		if (IS_ENABLED(CONFIG_PM_DEVICE) && pm_suspend_devices()) {
			return _handle_device_abort(z_power_state);
		}
		/*
		 * Disable idle exit notification as it is not needed
		 * in deep sleep mode.
		 */
		pm_idle_exit_notification_disable();
#if CONFIG_PM_DEVICE
	} else {
		if (pm_policy_low_power_devices(z_power_state.state)) {
			/* low power peripherals. */
			if (pm_low_power_devices()) {
				return _handle_device_abort(z_power_state);
			}

			low_power = true;
		}
#endif
	}

	pm_debug_start_timer();
	/* Enter power state */
	pm_power_state_set(z_power_state);
	pm_debug_stop_timer();

	/* Wake up sequence starts here */
#if CONFIG_PM_DEVICE
	if (deep_sleep || low_power) {
		/* Turn on peripherals and restore device states as necessary */
		pm_resume_devices();
	}
#endif
	pm_log_debug_info(z_power_state.state);

	if (!post_ops_done) {
		post_ops_done = 1;
		/* clear z_forced_power_state */
		z_forced_power_state = false;
		pm_state_notify(false);
		pm_power_state_exit_post_ops(z_power_state);
	}

	return z_power_state.state;
}


enum pm_state pm_system_suspend(int32_t ticks)
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
		pm_state_notify(false);
		pm_power_state_exit_post_ops(z_power_state);
	}
}

void pm_notifier_register(struct pm_notifier *notifier)
{
	k_spinlock_key_t pm_notifier_key = k_spin_lock(&pm_notifier_lock);

	sys_slist_append(&pm_notifiers, &notifier->_node);
	k_spin_unlock(&pm_notifier_lock, pm_notifier_key);
}

int pm_notifier_unregister(struct pm_notifier *notifier)
{
	int ret = -EINVAL;
	k_spinlock_key_t pm_notifier_key;

	pm_notifier_key = k_spin_lock(&pm_notifier_lock);
	if (sys_slist_find_and_remove(&pm_notifiers, &(notifier->_node))) {
		ret = 0;
	}
	k_spin_unlock(&pm_notifier_lock, pm_notifier_key);

	return ret;
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
