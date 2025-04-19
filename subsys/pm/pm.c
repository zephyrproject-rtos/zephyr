/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
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
#include "device_system_managed.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pm, CONFIG_PM_LOG_LEVEL);

static ATOMIC_DEFINE(z_post_ops_required, CONFIG_MP_MAX_NUM_CPUS);
static sys_slist_t pm_notifiers = SYS_SLIST_STATIC_INIT(&pm_notifiers);

/* Convert exit-latency-us to ticks using specified method. */
#define EXIT_LATENCY_US_TO_TICKS(us)						    \
	IS_ENABLED(CONFIG_PM_PREWAKEUP_CONV_MODE_NEAR) ? k_us_to_ticks_near32(us) : \
	IS_ENABLED(CONFIG_PM_PREWAKEUP_CONV_MODE_CEIL) ? k_us_to_ticks_ceil32(us) : \
		k_us_to_ticks_floor32(us)

/* State pointers which are set to NULL indicate ACTIVE state. */
static const struct pm_state_info *z_cpus_pm_state[CONFIG_MP_MAX_NUM_CPUS];
static const struct pm_state_info *z_cpus_pm_forced_state[CONFIG_MP_MAX_NUM_CPUS];

static struct k_spinlock pm_forced_state_lock;
static struct k_spinlock pm_notifier_lock;

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
			callback(z_cpus_pm_state[CPU_ID]->state);
		}
	}
	k_spin_unlock(&pm_notifier_lock, pm_notifier_key);
}

static inline int32_t ticks_expiring_sooner(int32_t ticks1, int32_t ticks2)
{
	/*
	 * Ticks are relative numbers that defines the number of ticks
	 * until the next event.
	 * Its maximum value is K_TICKS_FOREVER ((uint32_t)-1) which is -1
	 * when we cast it to (int32_t)
	 * We need to find out which one is the closest
	 */

	__ASSERT(ticks1 >= -1, "ticks1 has unexpected negative value");
	__ASSERT(ticks2 >= -1, "ticks2 has unexpected negative value");

	if (ticks1 == K_TICKS_FOREVER) {
		return ticks2;
	}
	if (ticks2 == K_TICKS_FOREVER) {
		return ticks1;
	}
	/* At this step ticks1 and ticks2 are positive */
	return MIN(ticks1, ticks2);
}

void pm_system_resume(void)
{
	uint8_t id = CPU_ID;

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
#ifdef CONFIG_PM_DEVICE_SYSTEM_MANAGED
		if (atomic_add(&_cpus_active, 1) == 0) {
			if ((z_cpus_pm_state[id]->state != PM_STATE_RUNTIME_IDLE) &&
					!z_cpus_pm_state[id]->pm_device_disabled) {
				pm_resume_devices();
			}
		}
#endif
		pm_state_exit_post_ops(z_cpus_pm_state[id]->state,
				       z_cpus_pm_state[id]->substate_id);
		pm_state_notify(false);
#ifdef CONFIG_SYS_CLOCK_EXISTS
		sys_clock_idle_exit();
#endif /* CONFIG_SYS_CLOCK_EXISTS */
		z_cpus_pm_state[id] = NULL;
	}
}

bool pm_state_force(uint8_t cpu, const struct pm_state_info *info)
{
	k_spinlock_key_t key;

	__ASSERT(info->state < PM_STATE_COUNT,
		 "Invalid power state %d!", info->state);

	info = pm_state_get(cpu, info->state, info->substate_id);

	key = k_spin_lock(&pm_forced_state_lock);
	z_cpus_pm_forced_state[cpu] = info;
	k_spin_unlock(&pm_forced_state_lock, key);

	return true;
}

bool pm_system_suspend(int32_t kernel_ticks)
{
	uint8_t id = CPU_ID;
	k_spinlock_key_t key;
	int32_t ticks, events_ticks;
	uint32_t exit_latency_ticks;

	SYS_PORT_TRACING_FUNC_ENTER(pm, system_suspend, kernel_ticks);

	if (!pm_policy_state_any_active()) {
		/* Return early if all states are unavailable. */
		return false;
	}

	/*
	 * CPU needs to be fully wake up before the event is triggered.
	 * We need to find out first the ticks to the next event
	 */
	events_ticks = pm_policy_next_event_ticks();
	ticks = ticks_expiring_sooner(kernel_ticks, events_ticks);

	key = k_spin_lock(&pm_forced_state_lock);
	if (z_cpus_pm_forced_state[id] != NULL) {
		z_cpus_pm_state[id] = z_cpus_pm_forced_state[id];
		z_cpus_pm_forced_state[id] = NULL;
	} else {
		z_cpus_pm_state[id] = pm_policy_next_state(id, ticks);
	}
	k_spin_unlock(&pm_forced_state_lock, key);

	if (z_cpus_pm_state[id] == NULL) {
		LOG_DBG("No PM operations done.");
		SYS_PORT_TRACING_FUNC_EXIT(pm, system_suspend, ticks, PM_STATE_ACTIVE);
		return false;
	}

#ifdef CONFIG_PM_DEVICE_SYSTEM_MANAGED
	if (atomic_sub(&_cpus_active, 1) == 1) {
		if ((z_cpus_pm_state[id]->state != PM_STATE_RUNTIME_IDLE) &&
		    !z_cpus_pm_state[id]->pm_device_disabled) {
			if (!pm_suspend_devices()) {
				pm_resume_devices();
				z_cpus_pm_state[id] = NULL;
				(void)atomic_add(&_cpus_active, 1);
				SYS_PORT_TRACING_FUNC_EXIT(pm, system_suspend, ticks,
							   PM_STATE_ACTIVE);
				return false;
			}
		}
	}
#endif

	exit_latency_ticks = EXIT_LATENCY_US_TO_TICKS(z_cpus_pm_state[id]->exit_latency_us);
	if ((exit_latency_ticks > 0) && (ticks != K_TICKS_FOREVER)) {
		/*
		 * We need to set the timer to interrupt a little bit early to
		 * accommodate the time required by the CPU to fully wake up.
		 */
		sys_clock_set_timeout(ticks - exit_latency_ticks, true);
	}

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
	pm_state_set(z_cpus_pm_state[id]->state, z_cpus_pm_state[id]->substate_id);
	pm_stats_stop();

	/* Wake up sequence starts here */
	pm_stats_update(z_cpus_pm_state[id]->state);
	pm_system_resume();
	k_sched_unlock();
	SYS_PORT_TRACING_FUNC_EXIT(pm, system_suspend, ticks,
				   z_cpus_pm_state[id] ?
				   z_cpus_pm_state[id]->state : PM_STATE_ACTIVE);

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
	static const struct pm_state_info active = {
		.state = PM_STATE_ACTIVE
	};

	return z_cpus_pm_state[cpu] ? z_cpus_pm_state[cpu] : &active;
}
