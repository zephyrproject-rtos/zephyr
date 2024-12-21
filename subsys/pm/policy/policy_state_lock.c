/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/toolchain.h>

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
