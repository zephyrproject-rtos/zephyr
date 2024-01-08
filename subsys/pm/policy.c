/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/toolchain.h>
#include <zephyr/pm/device.h>

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)

#define DT_SUB_LOCK_INIT(node_id)				\
	{ .state = PM_STATE_DT_INIT(node_id),			\
	  .substate_id = DT_PROP_OR(node_id, substate_id, 0),	\
	  .lock = ATOMIC_INIT(0),				\
	},

/**
 * State and substate lock structure.
 *
 * This struct is associating a reference counting to each <state,substate>
 * couple to be used with the pm_policy_substate_lock_* functions.
 *
 * Operations on this array are in the order of O(n) with the number of power
 * states and this is mostly due to the random nature of the substate value
 * (that can be anything from a small integer value to a bitmask). We can
 * probably do better with an hashmap.
 */
static struct {
	enum pm_state state;
	uint8_t substate_id;
	atomic_t lock;
} substate_lock_t[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_power_state, DT_SUB_LOCK_INIT)
};

#endif

/** Lock to synchronize access to the latency request list. */
static struct k_spinlock latency_lock;
/** List of maximum latency requests. */
static sys_slist_t latency_reqs;
/** Maximum CPU latency in us */
static int32_t max_latency_us = SYS_FOREVER_US;
/** Maximum CPU latency in cycles */
static int32_t max_latency_cyc = -1;
/** List of latency change subscribers. */
static sys_slist_t latency_subs;

/** Lock to synchronize access to the events list. */
static struct k_spinlock events_lock;
/** List of events. */
static sys_slist_t events_list;
/** Next event, in absolute cycles (<0: none, [0, UINT32_MAX]: cycles) */
static int64_t next_event_cyc = -1;

/** @brief Update maximum allowed latency. */
static void update_max_latency(void)
{
	int32_t new_max_latency_us = SYS_FOREVER_US;
	struct pm_policy_latency_request *req;

	SYS_SLIST_FOR_EACH_CONTAINER(&latency_reqs, req, node) {
		if ((new_max_latency_us == SYS_FOREVER_US) ||
		    ((int32_t)req->value_us < new_max_latency_us)) {
			new_max_latency_us = (int32_t)req->value_us;
		}
	}

	if (max_latency_us != new_max_latency_us) {
		struct pm_policy_latency_subscription *sreq;
		int32_t new_max_latency_cyc = -1;

		SYS_SLIST_FOR_EACH_CONTAINER(&latency_subs, sreq, node) {
			sreq->cb(new_max_latency_us);
		}

		if (new_max_latency_us != SYS_FOREVER_US) {
			new_max_latency_cyc = (int32_t)k_us_to_cyc_ceil32(new_max_latency_us);
		}

		max_latency_us = new_max_latency_us;
		max_latency_cyc = new_max_latency_cyc;
	}
}

/** @brief Update next event. */
static void update_next_event(uint32_t cyc)
{
	int64_t new_next_event_cyc = -1;
	struct pm_policy_event *evt;

	SYS_SLIST_FOR_EACH_CONTAINER(&events_list, evt, node) {
		uint64_t cyc_evt = evt->value_cyc;

		/*
		 * cyc value is a 32-bit rolling counter:
		 *
		 * |---------------->-----------------------|
		 * 0               cyc                  UINT32_MAX
		 *
		 * Values from [0, cyc) are events happening later than
		 * [cyc, UINT32_MAX], so pad [0, cyc) with UINT32_MAX + 1 to do
		 * the comparison.
		 */
		if (cyc_evt < cyc) {
			cyc_evt += UINT32_MAX + 1U;
		}

		if ((new_next_event_cyc < 0) ||
		    (cyc_evt < new_next_event_cyc)) {
			new_next_event_cyc = cyc_evt;
		}
	}

	/* undo padding for events in the [0, cyc) range */
	if (new_next_event_cyc > UINT32_MAX) {
		new_next_event_cyc -= UINT32_MAX + 1U;
	}

	next_event_cyc = new_next_event_cyc;
}

#ifdef CONFIG_PM_POLICY_DEFAULT
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	int64_t cyc = -1;
	uint8_t num_cpu_states;
	const struct pm_state_info *cpu_states;

#ifdef CONFIG_PM_NEED_ALL_DEVICES_IDLE
	if (pm_device_is_any_busy()) {
		return NULL;
	}
#endif

	if (ticks != K_TICKS_FOREVER) {
		cyc = k_ticks_to_cyc_ceil32(ticks);
	}

	num_cpu_states = pm_state_cpu_get_all(cpu, &cpu_states);

	if (next_event_cyc >= 0) {
		uint32_t cyc_curr = k_cycle_get_32();
		int64_t cyc_evt = next_event_cyc - cyc_curr;

		/* event happening after cycle counter max value, pad */
		if (next_event_cyc <= cyc_curr) {
			cyc_evt += UINT32_MAX;
		}

		if (cyc_evt > 0) {
			/* if there's no system wakeup event always wins,
			 * otherwise, who comes earlier wins
			 */
			if (cyc < 0) {
				cyc = cyc_evt;
			} else {
				cyc = MIN(cyc, cyc_evt);
			}
		}
	}

	for (int16_t i = (int16_t)num_cpu_states - 1; i >= 0; i--) {
		const struct pm_state_info *state = &cpu_states[i];
		uint32_t min_residency_cyc, exit_latency_cyc;

		/* check if there is a lock on state + substate */
		if (pm_policy_state_lock_is_active(state->state, state->substate_id)) {
			continue;
		}

		min_residency_cyc = k_us_to_cyc_ceil32(state->min_residency_us);
		exit_latency_cyc = k_us_to_cyc_ceil32(state->exit_latency_us);

		/* skip state if it brings too much latency */
		if ((max_latency_cyc >= 0) &&
		    (exit_latency_cyc >= max_latency_cyc)) {
			continue;
		}

		if ((cyc < 0) ||
		    (cyc >= (min_residency_cyc + exit_latency_cyc))) {
			return state;
		}
	}

	return NULL;
}
#endif

void pm_policy_state_lock_get(enum pm_state state, uint8_t substate_id)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	for (size_t i = 0; i < ARRAY_SIZE(substate_lock_t); i++) {
		if (substate_lock_t[i].state == state &&
		   (substate_lock_t[i].substate_id == substate_id ||
		    substate_id == PM_ALL_SUBSTATES)) {
			atomic_inc(&substate_lock_t[i].lock);
		}
	}
#endif
}

void pm_policy_state_lock_put(enum pm_state state, uint8_t substate_id)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	for (size_t i = 0; i < ARRAY_SIZE(substate_lock_t); i++) {
		if (substate_lock_t[i].state == state &&
		   (substate_lock_t[i].substate_id == substate_id ||
		    substate_id == PM_ALL_SUBSTATES)) {
			atomic_t cnt = atomic_dec(&substate_lock_t[i].lock);

			ARG_UNUSED(cnt);

			__ASSERT(cnt >= 1, "Unbalanced state lock get/put");
		}
	}
#endif
}

bool pm_policy_state_lock_is_active(enum pm_state state, uint8_t substate_id)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	for (size_t i = 0; i < ARRAY_SIZE(substate_lock_t); i++) {
		if (substate_lock_t[i].state == state &&
		   (substate_lock_t[i].substate_id == substate_id ||
		    substate_id == PM_ALL_SUBSTATES)) {
			return (atomic_get(&substate_lock_t[i].lock) != 0);
		}
	}
#endif

	return false;
}

void pm_policy_latency_request_add(struct pm_policy_latency_request *req,
				   uint32_t value_us)
{
	req->value_us = value_us;

	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	sys_slist_append(&latency_reqs, &req->node);
	update_max_latency();

	k_spin_unlock(&latency_lock, key);
}

void pm_policy_latency_request_update(struct pm_policy_latency_request *req,
				      uint32_t value_us)
{
	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	req->value_us = value_us;
	update_max_latency();

	k_spin_unlock(&latency_lock, key);
}

void pm_policy_latency_request_remove(struct pm_policy_latency_request *req)
{
	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	(void)sys_slist_find_and_remove(&latency_reqs, &req->node);
	update_max_latency();

	k_spin_unlock(&latency_lock, key);
}

void pm_policy_latency_changed_subscribe(struct pm_policy_latency_subscription *req,
					 pm_policy_latency_changed_cb_t cb)
{
	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	req->cb = cb;
	sys_slist_append(&latency_subs, &req->node);

	k_spin_unlock(&latency_lock, key);
}

void pm_policy_latency_changed_unsubscribe(struct pm_policy_latency_subscription *req)
{
	k_spinlock_key_t key = k_spin_lock(&latency_lock);

	(void)sys_slist_find_and_remove(&latency_subs, &req->node);

	k_spin_unlock(&latency_lock, key);
}

void pm_policy_event_register(struct pm_policy_event *evt, uint32_t time_us)
{
	k_spinlock_key_t key = k_spin_lock(&events_lock);
	uint32_t cyc = k_cycle_get_32();

	evt->value_cyc = cyc + k_us_to_cyc_ceil32(time_us);
	sys_slist_append(&events_list, &evt->node);
	update_next_event(cyc);

	k_spin_unlock(&events_lock, key);
}

void pm_policy_event_update(struct pm_policy_event *evt, uint32_t time_us)
{
	k_spinlock_key_t key = k_spin_lock(&events_lock);
	uint32_t cyc = k_cycle_get_32();

	evt->value_cyc = cyc + k_us_to_cyc_ceil32(time_us);
	update_next_event(cyc);

	k_spin_unlock(&events_lock, key);
}

void pm_policy_event_unregister(struct pm_policy_event *evt)
{
	k_spinlock_key_t key = k_spin_lock(&events_lock);

	(void)sys_slist_find_and_remove(&events_list, &evt->node);
	update_next_event(k_cycle_get_32());

	k_spin_unlock(&events_lock, key);
}
