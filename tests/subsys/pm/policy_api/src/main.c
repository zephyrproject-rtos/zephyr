/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/policy.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys_clock.h>
#include <zephyr/ztest.h>

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

ZTEST_SUITE(policy_api, NULL, NULL, NULL, NULL, NULL);
