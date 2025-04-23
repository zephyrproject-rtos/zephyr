/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/pm/pm.h>

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS == 2, "Invalid number of cpus");

#define NUM_OF_ITERATIONS (5)

#define THRESHOLD (10)

/* Arbitrary times to trigger different power states.  It is by the
 * application to sleep and the policy to define which power state
 * to use. They have no relationship with any platform.
 */
#define ACTIVE_MSEC (40)
#define ACTIVE_TIMEOUT K_MSEC(40)
#define IDLE_MSEC (60 + THRESHOLD)
#define IDLE_TIMEOUT K_MSEC(60)
#define SUSPEND_TO_IDLE_MSEC (100 + THRESHOLD)
#define SUSPEND_TO_IDLE_TIMEOUT K_MSEC(100)
#define STANDBY_TIMEOUT K_MSEC(200)


static enum pm_state state_testing[2];

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state_testing[_current_cpu->id]) {
	case  PM_STATE_RUNTIME_IDLE:
		zassert_equal(PM_STATE_RUNTIME_IDLE, state);
		break;
	case  PM_STATE_SUSPEND_TO_IDLE:
		zassert_equal(PM_STATE_SUSPEND_TO_IDLE, state);
		break;
	case  PM_STATE_STANDBY:
		zassert_equal(_current_cpu->id, 1U);
		zassert_equal(PM_STATE_STANDBY, state);
		break;
	default:
		zassert_unreachable(NULL);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	/* pm_system_suspend is entered with irq locked
	 * unlock irq before leave pm_system_suspend
	 */
	irq_unlock(0);
}

const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int ticks)
{
	static const struct pm_state_info states[] = {
		{
			.state = PM_STATE_ACTIVE
		},
		{
			.state = PM_STATE_RUNTIME_IDLE
		},
		{
			.state = PM_STATE_SUSPEND_TO_IDLE
		},
		{
			.state = PM_STATE_STANDBY
		},
	};
	static const struct pm_state_info *info;
	int32_t msecs = k_ticks_to_ms_floor64(ticks);

	if (msecs < ACTIVE_MSEC) {
		info = NULL;
	} else if (msecs <= IDLE_MSEC) {
		info = &states[1];
	} else if (msecs <= SUSPEND_TO_IDLE_MSEC) {
		info = &states[2];
	} else {
		if (cpu == 0U) {
			info = &states[2];
		} else {
			info = &states[3];
		}
	}

	state_testing[cpu] = info ? info->state : PM_STATE_ACTIVE;

	return info;
}

/*
 * @brief test power idle in multicore
 *
 * @details
 *  - Go over a list of timeouts that should trigger different sleep states
 *  - The test assumes there are two cpus. The cpu 1 has one deeper sleep state than cpu 0.
 *  - Checks that the states given by the policy are properly used in the idle thread.
 *  - Iterate a number of times to stress it.
 *
 * @see pm_policy_next_state()
 * @see pm_state_set()
 *
 * @ingroup power_tests
 */
ZTEST(pm_multicore, test_power_idle)
{

	for (uint8_t i = 0U; i < NUM_OF_ITERATIONS; i++) {
		k_sleep(ACTIVE_TIMEOUT);

		k_sleep(IDLE_TIMEOUT);

		k_sleep(SUSPEND_TO_IDLE_TIMEOUT);

		k_sleep(STANDBY_TIMEOUT);
	}
}

ZTEST_SUITE(pm_multicore, NULL, NULL, NULL, NULL, NULL);
