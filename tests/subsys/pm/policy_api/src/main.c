/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pm/policy.h>
#include <sys/time_units.h>
#include <sys_clock.h>
#include <ztest.h>

#ifdef CONFIG_PM_POLICY_RESIDENCY
/**
 * @brief Test the behavior of pm_policy_next_state() when
 * CONFIG_PM_POLICY_RESIDENCY=y.
 */
static void test_pm_policy_next_state_residency(void)
{
	const struct pm_state_info *next;

	/* cpu 0 */
	next = pm_policy_next_state(0U, 0);
	zassert_equal(next, NULL, NULL);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(10999));
	zassert_equal(next, NULL, NULL);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE, NULL);
	zassert_equal(next->min_residency_us, 100000, NULL);
	zassert_equal(next->exit_latency_us, 10000, NULL);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1099999));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE, NULL);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM, NULL);
	zassert_equal(next->min_residency_us, 1000000, NULL);
	zassert_equal(next->exit_latency_us, 100000, NULL);

	next = pm_policy_next_state(0U, K_TICKS_FOREVER);
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM, NULL);

	/* cpu 1 */
	next = pm_policy_next_state(1U, 0);
	zassert_equal(next, NULL, NULL);

	next = pm_policy_next_state(1U, k_us_to_ticks_floor32(549999));
	zassert_equal(next, NULL, NULL);

	next = pm_policy_next_state(1U, k_us_to_ticks_floor32(550000));
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM, NULL);
	zassert_equal(next->min_residency_us, 500000, NULL);
	zassert_equal(next->exit_latency_us, 50000, NULL);

	next = pm_policy_next_state(1U, K_TICKS_FOREVER);
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM, NULL);
}
#else
static void test_pm_policy_next_state_residency(void)
{
	ztest_test_skip();
}
#endif /* CONFIG_PM_POLICY_RESIDENCY */

#ifdef CONFIG_PM_POLICY_APP
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	static const struct pm_state_info state = {.state = PM_STATE_SOFT_OFF};

	ARG_UNUSED(cpu);
	ARG_UNUSED(ticks);

	return &state;
}

/**
 * @brief Test that a custom policy can be implemented when
 * CONFIG_PM_POLICY_APP=y.
 */
static void test_pm_policy_next_state_app(void)
{
	const struct pm_state_info *next;

	next = pm_policy_next_state(0U, 0);
	zassert_equal(next->state, PM_STATE_SOFT_OFF, NULL);
}
#else
static void test_pm_policy_next_state_app(void)
{
	ztest_test_skip();
}
#endif /* CONFIG_PM_POLICY_APP */

void test_main(void)
{
	ztest_test_suite(policy_api,
			 ztest_unit_test(test_pm_policy_next_state_residency),
			 ztest_unit_test(test_pm_policy_next_state_app));
	ztest_run_test_suite(policy_api);
}
