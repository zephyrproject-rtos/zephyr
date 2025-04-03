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
#include <zephyr/spinlock.h>

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)

#define DT_SUB_LOCK_INIT(node_id)					\
	{ .state = PM_STATE_DT_INIT(node_id),				\
	  .substate_id = DT_PROP_OR(node_id, substate_id, 0),		\
	  .exit_latency_us = DT_PROP_OR(node_id, exit_latency_us, 0),	\
	},

/**
 * State and substate lock structure.
 *
 * Struct holds all power states defined in the device tree. Array with counter
 * variables is in RAM and n-th counter is used for n-th power state. Structure
 * also holds exit latency for each state. It is used to disable power states
 * based on current latency requirement.
 *
 * Operations on this array are in the order of O(n) with the number of power
 * states and this is mostly due to the random nature of the substate value
 * (that can be anything from a small integer value to a bitmask). We can
 * probably do better with an hashmap.
 */
static const struct {
	enum pm_state state;
	uint8_t substate_id;
	uint32_t exit_latency_us;
} substates[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_power_state, DT_SUB_LOCK_INIT)
};
static atomic_t lock_cnt[ARRAY_SIZE(substates)];
static atomic_t latency_mask = BIT_MASK(ARRAY_SIZE(substates));
static atomic_t unlock_mask = BIT_MASK(ARRAY_SIZE(substates));
static struct k_spinlock lock;

#endif

void pm_policy_state_lock_get(enum pm_state state, uint8_t substate_id)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	for (size_t i = 0; i < ARRAY_SIZE(substates); i++) {
		if (substates[i].state == state &&
		   (substates[i].substate_id == substate_id || substate_id == PM_ALL_SUBSTATES)) {
			k_spinlock_key_t key = k_spin_lock(&lock);

			if (lock_cnt[i] == 0) {
				unlock_mask &= ~BIT(i);
			}
			lock_cnt[i]++;
			k_spin_unlock(&lock, key);
		}
	}
#endif
}

void pm_policy_state_lock_put(enum pm_state state, uint8_t substate_id)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	for (size_t i = 0; i < ARRAY_SIZE(substates); i++) {
		if (substates[i].state == state &&
		   (substates[i].substate_id == substate_id || substate_id == PM_ALL_SUBSTATES)) {
			k_spinlock_key_t key = k_spin_lock(&lock);

			__ASSERT(lock_cnt[i] > 0, "Unbalanced state lock get/put");
			lock_cnt[i]--;
			if (lock_cnt[i] == 0) {
				unlock_mask |= BIT(i);
			}
			k_spin_unlock(&lock, key);
		}
	}
#endif
}

bool pm_policy_state_lock_is_active(enum pm_state state, uint8_t substate_id)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	for (size_t i = 0; i < ARRAY_SIZE(substates); i++) {
		if (substates[i].state == state &&
		   (substates[i].substate_id == substate_id || substate_id == PM_ALL_SUBSTATES)) {
			return atomic_get(&lock_cnt[i]) != 0;
		}
	}
#endif

	return false;
}

bool pm_policy_state_is_available(enum pm_state state, uint8_t substate_id)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	for (size_t i = 0; i < ARRAY_SIZE(substates); i++) {
		if (substates[i].state == state &&
		   (substates[i].substate_id == substate_id || substate_id == PM_ALL_SUBSTATES)) {
			return (atomic_get(&lock_cnt[i]) == 0) &&
			       (atomic_get(&latency_mask) & BIT(i));
		}
	}
#endif

	return false;
}

bool pm_policy_state_any_active(void)
{
#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
	/* Check if there is any power state that is not locked and not disabled due
	 * to latency requirements.
	 */
	return atomic_get(&unlock_mask) & atomic_get(&latency_mask);
#endif
	return true;
}

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_power_state)
/* Callback is called whenever latency requirement changes. It is called under lock. */
static void pm_policy_latency_update_locked(int32_t max_latency_us)
{
	for (size_t i = 0; i < ARRAY_SIZE(substates); i++) {
		if (substates[i].exit_latency_us >= max_latency_us) {
			latency_mask &= ~BIT(i);
		} else {
			latency_mask |= BIT(i);
		}
	}
}

static int pm_policy_latency_init(void)
{
	static struct pm_policy_latency_subscription sub;

	pm_policy_latency_changed_subscribe(&sub, pm_policy_latency_update_locked);

	return 0;
}

SYS_INIT(pm_policy_latency_init, PRE_KERNEL_1, 0);
#endif
