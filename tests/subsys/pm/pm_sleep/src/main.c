/*
 * Copyright (c) 2026 Rithic Chellaram Hariharan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>
#include <zephyr/ztest.h>

#define SLEEP_TIMEOUT K_MSEC(20)

/* Records the last state the (stubbed) SoC backend was asked to enter. */
static enum pm_state last_state;
static uint32_t enter_count;

/* Stubbed SoC power hooks. native_sim has no real backend. */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	last_state = state;
	enter_count++;
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	irq_unlock(0);
}

/* Only enter explicitly forced states (deterministic counts). */
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	ARG_UNUSED(cpu);
	ARG_UNUSED(ticks);

	return NULL;
}

static void before(void *unused)
{
	ARG_UNUSED(unused);

	last_state = PM_STATE_ACTIVE;
	enter_count = 0;
}

/* Light sleep enters the shallowest context-retaining state (runtime-idle). */
ZTEST(pm_sleep, test_light_sleep)
{
	int ret = pm_light_sleep(SLEEP_TIMEOUT);

	if (IS_ENABLED(CONFIG_APP_HAVE_RETAINING_STATE)) {
		zassert_equal(ret, 0, "pm_light_sleep() returned %d", ret);
		zassert_equal(enter_count, 1, "expected one state entry, got %u", enter_count);
		zassert_equal(last_state, PM_STATE_RUNTIME_IDLE, "expected runtime-idle, got %s",
			      pm_state_to_str(last_state));
	} else {
		zassert_equal(ret, -ENOTSUP, "expected -ENOTSUP, got %d", ret);
		zassert_equal(enter_count, 0, "no state should be entered, got %u", enter_count);
	}
}

/* Deep sleep enters the deepest context-retaining state (suspend-to-ram),
 * never soft-off.
 */
ZTEST(pm_sleep, test_deep_sleep)
{
	int ret = pm_deep_sleep(SLEEP_TIMEOUT);

	if (IS_ENABLED(CONFIG_APP_HAVE_RETAINING_STATE)) {
		zassert_equal(ret, 0, "pm_deep_sleep() returned %d", ret);
		zassert_equal(enter_count, 1, "expected one state entry, got %u", enter_count);
		zassert_equal(last_state, PM_STATE_SUSPEND_TO_RAM,
			      "expected suspend-to-ram, got %s", pm_state_to_str(last_state));
	} else {
		zassert_equal(ret, -ENOTSUP, "expected -ENOTSUP, got %d", ret);
		zassert_equal(enter_count, 0, "no state should be entered, got %u", enter_count);
	}
}

/* Soft-off enters PM_STATE_SOFT_OFF even with no context-retaining state. */
ZTEST(pm_sleep, test_soft_off)
{
	int ret = pm_soft_off(SLEEP_TIMEOUT);

	zassert_equal(ret, 0, "pm_soft_off() returned %d", ret);
	zassert_equal(enter_count, 1, "expected one state entry, got %u", enter_count);
	zassert_equal(last_state, PM_STATE_SOFT_OFF, "expected soft-off, got %s",
		      pm_state_to_str(last_state));
}

ZTEST_SUITE(pm_sleep, NULL, NULL, before, NULL, NULL);
