/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/timeout_q.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/state.h>
#include <zephyr/pm/policy.h>
#include <zephyr/tracing/tracing.h>

#include "pm_stats.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pm, CONFIG_PM_LOG_LEVEL);

#define CURRENT_CPU \
	(COND_CODE_1(CONFIG_SMP, (arch_curr_cpu()->id), (_current_cpu->id)))

static ATOMIC_DEFINE(z_post_ops_required, CONFIG_MP_MAX_NUM_CPUS);
static sys_slist_t pm_notifiers = SYS_SLIST_STATIC_INIT(&pm_notifiers);

/*
 * Properly initialize cpu power states. Do not make assumptions that
 * ACTIVE_STATE is 0
 */
#define CPU_PM_STATE_INIT(_, __)		\
	{ .state = PM_STATE_ACTIVE }
static struct pm_state_info z_cpus_pm_state[] = {
	LISTIFY(CONFIG_MP_MAX_NUM_CPUS, CPU_PM_STATE_INIT, (,))
};

static struct pm_state_info z_cpus_pm_forced_state[] = {
	LISTIFY(CONFIG_MP_MAX_NUM_CPUS, CPU_PM_STATE_INIT, (,))
};

static struct k_spinlock pm_forced_state_lock;

#if defined(CONFIG_PM_DEVICE) && !defined(CONFIG_PM_DEVICE_RUNTIME_EXCLUSIVE)
static atomic_t z_cpus_active = ATOMIC_INIT(CONFIG_MP_MAX_NUM_CPUS);
#endif
static struct k_spinlock pm_notifier_lock;


#ifdef CONFIG_PM_DEVICE
TYPE_SECTION_START_EXTERN(const struct device *, pm_device_slots);

#if !defined(CONFIG_PM_DEVICE_RUNTIME_EXCLUSIVE)
/* Number of devices successfully suspended. */
static size_t num_susp;

static int pm_suspend_devices(void)
{
	const struct device *devs;
	size_t devc;

	devc = z_device_get_all_static(&devs);

	num_susp = 0;

	for (const struct device *dev = devs + devc - 1; dev >= devs; dev--) {
		int ret;

		/*
		 * Ignore uninitialized devices, busy devices, wake up sources, and
		 * devices with runtime PM enabled.
		 */
		if (!device_is_ready(dev) || pm_device_is_busy(dev) ||
		    pm_device_state_is_locked(dev) ||
		    pm_device_wakeup_is_enabled(dev) ||
		    pm_device_runtime_is_enabled(dev)) {
			continue;
		}

		ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
		/* ignore devices not supporting or already at the given state */
		if ((ret == -ENOSYS) || (ret == -ENOTSUP) || (ret == -EALREADY)) {
			continue;
		} else if (ret < 0) {
			LOG_ERR("Device %s did not enter %s state (%d)",
				dev->name,
				pm_device_state_str(PM_DEVICE_STATE_SUSPENDED),
				ret);
			return ret;
		}

		TYPE_SECTION_START(pm_device_slots)[num_susp] = dev;
		num_susp++;
	}

	return 0;
}

static void pm_resume_devices(void)
{
	for (int i = (num_susp - 1); i >= 0; i--) {
		pm_device_action_run(TYPE_SECTION_START(pm_device_slots)[i],
				    PM_DEVICE_ACTION_RESUME);
	}

	num_susp = 0;
}
#endif  /* !CONFIG_PM_DEVICE_RUNTIME_EXCLUSIVE */
#endif	/* CONFIG_PM_DEVICE */

static inline void pm_exit_pos_ops(struct pm_state_info *info)
{
	extern __weak void
		pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id);

	if (pm_state_exit_post_ops != NULL) {
		pm_state_exit_post_ops(info->state, info->substate_id);
	} else {
		/*
		 * This function is supposed to be overridden to do SoC or
		 * architecture specific post ops after sleep state exits.
		 *
		 * The kernel expects that irqs are unlocked after this.
		 */

		irq_unlock(0);
	}
}

static inline void state_set(struct pm_state_info *info)
{
	extern __weak void
		pm_state_set(enum pm_state state, uint8_t substate_id);

	if (pm_state_set != NULL) {
		pm_state_set(info->state, info->substate_id);
	}
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
			callback(z_cpus_pm_state[_current_cpu->id].state);
		}
	}
	k_spin_unlock(&pm_notifier_lock, pm_notifier_key);
}

void pm_system_resume(void)
{
	uint8_t id = CURRENT_CPU;

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
	 */
	if (atomic_test_and_clear_bit(z_post_ops_required, id)) {
		pm_exit_pos_ops(&z_cpus_pm_state[id]);
		pm_state_notify(false);
		z_cpus_pm_state[id] = (struct pm_state_info){PM_STATE_ACTIVE,
			0, 0};
	}
}

bool pm_state_force(uint8_t cpu, const struct pm_state_info *info)
{
	k_spinlock_key_t key;

	__ASSERT(info->state < PM_STATE_COUNT,
		 "Invalid power state %d!", info->state);

	key = k_spin_lock(&pm_forced_state_lock);
	z_cpus_pm_forced_state[cpu] = *info;
	k_spin_unlock(&pm_forced_state_lock, key);

	return true;
}

bool pm_system_suspend(int32_t ticks)
{
	uint8_t id = CURRENT_CPU;
	k_spinlock_key_t key;

	SYS_PORT_TRACING_FUNC_ENTER(pm, system_suspend, ticks);

	key = k_spin_lock(&pm_forced_state_lock);
	if (z_cpus_pm_forced_state[id].state != PM_STATE_ACTIVE) {
		z_cpus_pm_state[id] = z_cpus_pm_forced_state[id];
		z_cpus_pm_forced_state[id].state = PM_STATE_ACTIVE;
	} else {
		const struct pm_state_info *info;

		info = pm_policy_next_state(id, ticks);
		if (info != NULL) {
			z_cpus_pm_state[id] = *info;
		}
	}
	k_spin_unlock(&pm_forced_state_lock, key);

	if (z_cpus_pm_state[id].state == PM_STATE_ACTIVE) {
		LOG_DBG("No PM operations done.");
		SYS_PORT_TRACING_FUNC_EXIT(pm, system_suspend, ticks,
				   z_cpus_pm_state[id].state);
		return false;
	}

	if (ticks != K_TICKS_FOREVER) {
		/*
		 * We need to set the timer to interrupt a little bit early to
		 * accommodate the time required by the CPU to fully wake up.
		 */
		sys_clock_set_timeout(ticks -
		     k_us_to_ticks_ceil32(
			     z_cpus_pm_state[id].exit_latency_us),
				     true);
	}

#if defined(CONFIG_PM_DEVICE) && !defined(CONFIG_PM_DEVICE_RUNTIME_EXCLUSIVE)
	if (atomic_sub(&z_cpus_active, 1) == 1) {
		if (z_cpus_pm_state[id].state != PM_STATE_RUNTIME_IDLE) {
			if (pm_suspend_devices()) {
				pm_resume_devices();
				z_cpus_pm_state[id].state = PM_STATE_ACTIVE;
				(void)atomic_add(&z_cpus_active, 1);
				SYS_PORT_TRACING_FUNC_EXIT(pm, system_suspend, ticks,
							   z_cpus_pm_state[id].state);
				return false;
			}
		}
	}
#endif
	/*
	 * This function runs with interruptions locked but it is
	 * expected the SoC to unlock them in
	 * pm_state_exit_post_ops() when returning to active
	 * state. We don't want to be scheduled out yet, first we need
	 * to send a notification about leaving the idle state. So,
	 * we lock the scheduler here and unlock just after we have
	 * sent the notification in pm_system_resume().
	 */
	k_sched_lock();
	pm_stats_start();
	/* Enter power state */
	pm_state_notify(true);
	atomic_set_bit(z_post_ops_required, id);
	state_set(&z_cpus_pm_state[id]);
	pm_stats_stop();

	/* Wake up sequence starts here */
#if defined(CONFIG_PM_DEVICE) && !defined(CONFIG_PM_DEVICE_RUNTIME_EXCLUSIVE)
	if (atomic_add(&z_cpus_active, 1) == 0) {
		pm_resume_devices();
	}
#endif
	pm_stats_update(z_cpus_pm_state[id].state);
	pm_system_resume();
	k_sched_unlock();
	SYS_PORT_TRACING_FUNC_EXIT(pm, system_suspend, ticks,
				   z_cpus_pm_state[id].state);

	return true;
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

const struct pm_state_info *pm_state_next_get(uint8_t cpu)
{
	return &z_cpus_pm_state[cpu];
}
