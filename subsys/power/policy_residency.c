/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <pm_policy.h>
#include <sys/atomic.h>
#include <toolchain/gcc.h>

static atomic_t force_pm_state;
static atomic_t pm_state_constraints[PM_STATE_MAX];

static inline bool ticks_compare(int ticks, uint32_t residency)
{
	if (ticks == K_TICKS_FOREVER) {
		return true;
	}

	residency *= (CONFIG_SYS_CLOCK_TICKS_PER_SEC / MSEC_PER_SEC);
	return ticks >= residency;
}

/* compare the given ticks and the residency of given state
 * return true if given ticks is great or equal to the residency
 * of given state; otherwise return false.
 */
static inline bool residency_policy_compare(pm_state_t state, int ticks)
{
	switch (state & PM_STATE_BIT_MASK) {
		case PM_STATE_RUNTIME_IDLE:
			return ticks_compare(ticks,
					     CONFIG_RUNTIME_IDLE_RESIDENCY);
		case PM_STATE_SUSPEND_TO_IDLE:
			return ticks_compare(ticks,
					     CONFIG_SUSPEND_TO_IDLE_RESIDENCY);
		case PM_STATE_STANDBY:
			return ticks_compare(ticks, CONFIG_STANDBY_RESIDENCY);
		case PM_STATE_SUSPEND_TO_RAM:
			return ticks_compare(ticks,
					     CONFIG_SUSPEND_TO_RAM_RESIDENCY);
		case PM_STATE_SUSPEND_TO_DISK:
			return ticks_compare(ticks,
					     CONFIG_SUSPEND_TO_DISK_RESIDENCY);
		default:
			return false;
	}

	return false;
}

static void residency_policy_init(void)
{
	atomic_set(&force_pm_state, 0);

	for (int i = 0; i < ARRAY_SIZE(pm_state_constraints); i++) {
		atomic_set(&pm_state_constraints[i], 0);
	}
}

static pm_state_t residency_policy_next_state(struct pm_policy *policy,
					      int ticks)
{
	uint32_t index;
	pm_state_t states;

	if (atomic_get(&force_pm_state)) {
		return (pm_state_t)force_pm_state;
	}

	states = pm_policy_get_supported_state(policy);
	if (states) {
		while ((index = find_msb_set((uint32_t)states))) {
			index--;
			if (atomic_get(&pm_state_constraints[index]) == 0
			    && residency_policy_compare(BIT(index), ticks)) {
				return states & BIT(index);
			}
			states &= ~BIT(index);
		}
	}

	return PM_STATE_RUNTIME_ACTIVE;
}

static int residency_policy_set_force_state(struct pm_policy *policy,
					    pm_state_t state)
{
	state &= pm_policy_get_supported_state(policy);
	if (state) {
		atomic_set(&force_pm_state, state);
		return 0;
	}

	return -1;
}

static int residency_policy_clear_force_state(struct pm_policy *policy)
{
	ARG_UNUSED(policy);

	atomic_set(&force_pm_state, 0);
	return 0;
}

static int residency_policy_set_constraint(struct pm_policy *policy,
					   pm_state_t states)
{
	uint32_t index;

	states &= pm_policy_get_supported_state(policy);
	if (states) {
		while ((index = find_lsb_set((uint32_t)states))) {
			index--;
			atomic_inc(&pm_state_constraints[index]);
			states &= ~BIT(index);
		}
		return 0;
	}

	return -1;
}

static int residency_policy_release_constraint(struct pm_policy *policy,
					       pm_state_t states)
{
	uint32_t index;

	states &= pm_policy_get_supported_state(policy);
	if (states) {
		while ((index = find_lsb_set((uint32_t)states))) {
			index--;
			atomic_dec(&pm_state_constraints[index]);
			states &= ~BIT(index);
		}
		return 0;
	}

	return -1;
}

static const struct pm_policy_api residency_policy_api = {
	.init = residency_policy_init,
	.next_state = residency_policy_next_state,
	.set_force_state = residency_policy_set_force_state,
	.clear_force_state = residency_policy_clear_force_state,
	.set_constraint = residency_policy_set_constraint,
	.release_constraint = residency_policy_release_constraint
};

PM_POLICY_DEFINE(DEFAULT_RESIDENCY_POLICY_SUPPORTED_STATES,
		 default_residency_policy, residency_policy_api);
