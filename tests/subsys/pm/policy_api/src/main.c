/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/policy.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys_clock.h>
#include <zephyr/ztest.h>

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	ARG_UNUSED(state);
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	irq_unlock(0);
}

#ifdef CONFIG_PM_POLICY_DEFAULT
/**
 * @brief Test the behavior of pm_policy_next_state() when
 * CONFIG_PM_POLICY_DEFAULT=y.
 */
ZTEST(policy_api, test_pm_policy_next_state_default)
{
	const struct pm_state_info *next;

	/* cpu 0 */
	next = pm_policy_next_state(0U, 0);
	zassert_is_null(next);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(10999));
	zassert_is_null(next);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);
	zassert_equal(next->min_residency_us, 100000);
	zassert_equal(next->exit_latency_us, 10000);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1099999));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM);
	zassert_equal(next->min_residency_us, 1000000);
	zassert_equal(next->exit_latency_us, 100000);

	next = pm_policy_next_state(0U, K_TICKS_FOREVER);
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM);

	/* cpu 1 */
	next = pm_policy_next_state(1U, 0);
	zassert_is_null(next);

	next = pm_policy_next_state(1U, k_us_to_ticks_floor32(549999));
	zassert_is_null(next);

	next = pm_policy_next_state(1U, k_us_to_ticks_floor32(550000));
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM);
	zassert_equal(next->min_residency_us, 500000);
	zassert_equal(next->exit_latency_us, 50000);

	next = pm_policy_next_state(1U, K_TICKS_FOREVER);
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM);
}

ZTEST(policy_api, test_pm_policy_state_all_lock)
{
	/* initial state: PM_STATE_RUNTIME_IDLE allowed */
	zassert_false(pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES));
	zassert_true(pm_policy_state_is_available(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES));
	zassert_true(pm_policy_state_any_active());

	/* Locking all states. */
	pm_policy_state_all_lock_get();
	pm_policy_state_all_lock_get();

	/* States are locked. */
	zassert_true(pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES));
	zassert_false(pm_policy_state_is_available(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES));
	zassert_false(pm_policy_state_any_active());

	pm_policy_state_all_lock_put();

	/* States are still locked due to reference counter. */
	zassert_true(pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES));
	zassert_false(pm_policy_state_is_available(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES));
	zassert_false(pm_policy_state_any_active());

	pm_policy_state_all_lock_put();

	/* States are available again. */
	zassert_false(pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES));
	zassert_true(pm_policy_state_is_available(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES));
	zassert_true(pm_policy_state_any_active());
}

/**
 * @brief Test the behavior of pm_policy_next_state() when
 * states are allowed/disallowed and CONFIG_PM_POLICY_DEFAULT=y.
 */
ZTEST(policy_api, test_pm_policy_next_state_default_allowed)
{
	bool active;
	const struct pm_state_info *next;

	/* initial state: PM_STATE_RUNTIME_IDLE allowed
	 * next state: PM_STATE_RUNTIME_IDLE
	 */
	active = pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	zassert_false(active);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	/* disallow PM_STATE_RUNTIME_IDLE
	 * next state: NULL (active)
	 */
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);

	active = pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	zassert_true(active);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_is_null(next);

	/* allow PM_STATE_RUNTIME_IDLE again
	 * next state: PM_STATE_RUNTIME_IDLE
	 */
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);

	active = pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	zassert_false(active);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	/* initial state: PM_STATE_RUNTIME_IDLE and substate 1 allowed
	 * next state: PM_STATE_RUNTIME_IDLE
	 */
	pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, 1);
	zassert_false(active);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	/* disallow PM_STATE_RUNTIME_IDLE and substate 1
	 * next state: NULL (active)
	 */
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, 1);

	active = pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, 1);
	zassert_true(active);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_is_null(next);

	/* allow PM_STATE_RUNTIME_IDLE and substate 1 again
	 * next state: PM_STATE_RUNTIME_IDLE
	 */
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, 1);

	active = pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, 1);
	zassert_false(active);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);
}

/** Flag to indicate number of times callback has been called */
static uint8_t latency_cb_call_cnt;
/** Flag to indicate expected latency */
static int32_t expected_latency;

/**
 * Callback to notify when state allowed status changes.
 */
static void on_pm_policy_latency_changed(int32_t latency)
{
	TC_PRINT("Latency changed to %d\n", latency);

	zassert_equal(latency, expected_latency);

	latency_cb_call_cnt++;
}

/**
 * @brief Test the behavior of pm_policy_next_state() when
 * latency requirements are imposed and CONFIG_PM_POLICY_DEFAULT=y.
 */
ZTEST(policy_api, test_pm_policy_next_state_default_latency)
{
	struct pm_policy_latency_request req1, req2;
	struct pm_policy_latency_subscription sreq1, sreq2;
	const struct pm_state_info *next;

	/* add a latency requirement with a maximum value below the
	 * latency given by any state, so we should stay active all the time
	 */
	pm_policy_latency_request_add(&req1, 9000);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_is_null(next);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_is_null(next);

	/* update latency requirement to a value between latencies for
	 * PM_STATE_RUNTIME_IDLE and PM_STATE_SUSPEND_TO_RAM, so we should
	 * never enter PM_STATE_SUSPEND_TO_RAM.
	 */
	pm_policy_latency_request_update(&req1, 50000);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	/* add a new latency requirement with a maximum value below the
	 * latency given by any state, so we should stay active all the time
	 * since it overrides the previous one.
	 */
	pm_policy_latency_request_add(&req2, 8000);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_is_null(next);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_is_null(next);

	/* remove previous request, so we should recover behavior given by
	 * first request.
	 */
	pm_policy_latency_request_remove(&req2);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	/* remove first request, so we should observe regular behavior again */
	pm_policy_latency_request_remove(&req1);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM);

	/* get notified when latency requirement changes */
	pm_policy_latency_changed_subscribe(&sreq1, on_pm_policy_latency_changed);
	pm_policy_latency_changed_subscribe(&sreq2, on_pm_policy_latency_changed);

	/* add new request (expected notification) */
	latency_cb_call_cnt = 0;
	expected_latency = 10000;
	pm_policy_latency_request_add(&req1, 10000);
	zassert_equal(latency_cb_call_cnt, 2);

	/* update request (expected notification, but only sreq1) */
	pm_policy_latency_changed_unsubscribe(&sreq2);

	latency_cb_call_cnt = 0;
	expected_latency = 50000;
	pm_policy_latency_request_update(&req1, 50000);
	zassert_equal(latency_cb_call_cnt, 1);

	/* add a new request, with higher value (no notification, previous
	 * prevails)
	 */
	latency_cb_call_cnt = 0;
	pm_policy_latency_request_add(&req2, 60000);
	zassert_equal(latency_cb_call_cnt, 0);

	pm_policy_latency_request_remove(&req2);
	zassert_equal(latency_cb_call_cnt, 0);

	/* remove first request, we no longer have latency requirements */
	expected_latency = SYS_FOREVER_US;
	pm_policy_latency_request_remove(&req1);
	zassert_equal(latency_cb_call_cnt, 1);
}

/**
 * @brief Test pm_policy_state_constraints_get/put functions using devicetree
 * test-states property and PM_STATE_CONSTRAINTS macros.
 */
ZTEST(policy_api, test_pm_policy_state_constraints)
{
	/* Define constraints list from the zephyr,user test-states property */
	PM_STATE_CONSTRAINTS_LIST_DEFINE(DT_PATH(zephyr_user), test_states);

	/* Get the constraints structure from devicetree */
	struct pm_state_constraints test_constraints =
		PM_STATE_CONSTRAINTS_GET(DT_PATH(zephyr_user), test_states);

	/* Verify the constraints were parsed correctly from devicetree
	 * test-states = <&state0 &state2> from app.overlay
	 */
	zassert_equal(test_constraints.count, 2,
		      "Expected 2 constraints from test-states property");

	/* Check that the constraints contain the expected states:
	 * state0 (runtime-idle, substate 1) and state2 (suspend-to-ram, substate 100)
	 */
	bool found_runtime_idle = false;
	bool found_suspend_to_ram = false;

	for (size_t i = 0; i < test_constraints.count; i++) {
		TC_PRINT("Constraint %zu: state=%d, substate_id=%d\n",
			 i, test_constraints.list[i].state, test_constraints.list[i].substate_id);

		if (test_constraints.list[i].state == PM_STATE_RUNTIME_IDLE &&
		    test_constraints.list[i].substate_id == 1) {
			found_runtime_idle = true;
		}
		if (test_constraints.list[i].state == PM_STATE_SUSPEND_TO_RAM &&
		    test_constraints.list[i].substate_id == 100) {
			found_suspend_to_ram = true;
		}
	}

	zassert_true(found_runtime_idle,
		     "Expected runtime-idle state with substate 1 in constraints");
	zassert_true(found_suspend_to_ram,
		     "Expected suspend-to-ram state with substate 100 in constraints");

	/* Test that states are initially available */
	zassert_false(pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, 1),
		      "runtime-idle substate 1 should be initially available");
	zassert_false(pm_policy_state_lock_is_active(PM_STATE_SUSPEND_TO_RAM, 100),
		      "suspend-to-ram substate 100 should be initially available");

	/* Apply the constraints - this should lock the specified states */
	pm_policy_state_constraints_get(&test_constraints);

	/* Verify that the constrained states are now locked */
	zassert_true(pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, 1),
		     "runtime-idle substate 1 should be locked after applying constraints");
	zassert_true(pm_policy_state_lock_is_active(PM_STATE_SUSPEND_TO_RAM, 100),
		     "suspend-to-ram substate 100 should be locked after applying constraints");

	/* Verify that non-constrained states are still available */
	zassert_false(pm_policy_state_lock_is_active(PM_STATE_SUSPEND_TO_RAM, 10),
		      "suspend-to-ram substate 10 should not be locked");

	/* Test that policy respects the constraints */
	const struct pm_state_info *next;

	/* This should not return the locked runtime-idle state,
	 * but should return suspend-to-ram substate 10
	 */
	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_not_null(next, "Policy should return an available state");
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM);
	zassert_equal(next->substate_id, 10);

	/* Remove the constraints - this should unlock the states */
	pm_policy_state_constraints_put(&test_constraints);

	/* Verify that the previously constrained states are now unlocked */
	zassert_false(pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, 1),
		      "runtime-idle substate 1 should be unlocked after removing constraints");
	zassert_false(pm_policy_state_lock_is_active(PM_STATE_SUSPEND_TO_RAM, 100),
		      "suspend-to-ram substate 100 should be unlocked after removing constraints");

	/* Verify policy works normally again */
	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_not_null(next, "Policy should return a state after removing constraints");
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	/* Test multiple get/put cycles to verify reference counting */
	pm_policy_state_constraints_get(&test_constraints);
	pm_policy_state_constraints_get(&test_constraints);

	zassert_true(pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, 1),
		     "runtime-idle substate 1 should remain locked with multiple gets");

	/* First put should not unlock (reference count > 1) */
	pm_policy_state_constraints_put(&test_constraints);
	zassert_true(pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, 1),
		     "runtime-idle substate 1 should remain locked after first put");

	/* Second put should unlock (reference count = 0) */
	pm_policy_state_constraints_put(&test_constraints);
	zassert_false(pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE, 1),
		      "runtime-idle substate 1 should be unlocked after final put");
}
#else
ZTEST(policy_api, test_pm_policy_next_state_default)
{
	ztest_test_skip();
}

ZTEST(policy_api, test_pm_policy_next_state_default_allowed)
{
	ztest_test_skip();
}

ZTEST(policy_api, test_pm_policy_next_state_default_latency)
{
	ztest_test_skip();
}

ZTEST(policy_api, test_pm_policy_state_constraints)
{
	ztest_test_skip();
}
#endif /* CONFIG_PM_POLICY_DEFAULT */

#ifdef CONFIG_PM_POLICY_CUSTOM
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	static const struct pm_state_info state = {.state = PM_STATE_SOFT_OFF};

	ARG_UNUSED(cpu);
	ARG_UNUSED(ticks);

	return &state;
}

/**
 * @brief Test that a custom policy can be implemented when
 * CONFIG_PM_POLICY_CUSTOM=y.
 */
ZTEST(policy_api, test_pm_policy_next_state_custom)
{
	const struct pm_state_info *next;

	next = pm_policy_next_state(0U, 0);
	zassert_equal(next->state, PM_STATE_SOFT_OFF);
}
#else
ZTEST(policy_api, test_pm_policy_next_state_custom)
{
	ztest_test_skip();
}
#endif /* CONFIG_PM_POLICY_CUSTOM */

ZTEST(policy_api, test_pm_policy_events)
{
	struct pm_policy_event evt1;
	struct pm_policy_event evt2;
	int64_t now_uptime_ticks;
	int64_t evt1_1_uptime_ticks;
	int64_t evt1_2_uptime_ticks;
	int64_t evt2_uptime_ticks;

	now_uptime_ticks = k_uptime_ticks();
	evt1_1_uptime_ticks = now_uptime_ticks + 100;
	evt1_2_uptime_ticks = now_uptime_ticks + 200;
	evt2_uptime_ticks = now_uptime_ticks + 2000;

	zassert_equal(pm_policy_next_event_ticks(), -1);
	pm_policy_event_register(&evt1, evt1_1_uptime_ticks);
	pm_policy_event_register(&evt2, evt2_uptime_ticks);
	zassert_within(pm_policy_next_event_ticks(), 100, 50);
	pm_policy_event_unregister(&evt1);
	zassert_within(pm_policy_next_event_ticks(), 2000, 50);
	pm_policy_event_unregister(&evt2);
	zassert_equal(pm_policy_next_event_ticks(), -1);
	pm_policy_event_register(&evt2, evt2_uptime_ticks);
	zassert_within(pm_policy_next_event_ticks(), 2000, 50);
	pm_policy_event_register(&evt1, evt1_1_uptime_ticks);
	zassert_within(pm_policy_next_event_ticks(), 100, 50);
	pm_policy_event_update(&evt1, evt1_2_uptime_ticks);
	zassert_within(pm_policy_next_event_ticks(), 200, 50);
	pm_policy_event_unregister(&evt1);
	pm_policy_event_unregister(&evt2);
}

ZTEST_SUITE(policy_api, NULL, NULL, NULL, NULL, NULL);
