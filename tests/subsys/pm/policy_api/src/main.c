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

/** Flag to indicate expected latency */
static int32_t expected_latency;

/**
 * @brief Test the behavior of pm_policy_next_state() when
 * latency requirements are imposed and CONFIG_PM_POLICY_DEFAULT=y.
 */
ZTEST(policy_api, test_pm_policy_next_state_default_latency)
{
	struct pm_policy_latency_request req1, req2;
	const struct pm_state_info *next;
	int rv;

	rv = pm_policy_latency_immediate_ctrl_add(NULL);
	zassert_equal(rv, 0);

	memset(&req1, 0, sizeof(req1));
	memset(&req2, 0, sizeof(req2));

	/* add a latency requirement with a maximum value below the
	 * latency given by any state, so we should stay active all the time
	 */
	rv = pm_policy_latency_request_add(&req1, 9000);
	zassert_equal(rv, 0);

	/* Cannot add already added request. */
	rv = pm_policy_latency_request_add(&req1, 1000);
	zassert_equal(rv, -EALREADY);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_is_null(next);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_is_null(next);

	/* update latency requirement to a value between latencies for
	 * PM_STATE_RUNTIME_IDLE and PM_STATE_SUSPEND_TO_RAM, so we should
	 * never enter PM_STATE_SUSPEND_TO_RAM.
	 */
	rv = pm_policy_latency_request_update(&req1, 50000);
	zassert_equal(rv, 0);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE);

	/* add a new latency requirement with a maximum value below the
	 * latency given by any state, so we should stay active all the time
	 * since it overrides the previous one.
	 */
	rv = pm_policy_latency_request_add(&req2, 8000);
	zassert_equal(rv, 0);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_is_null(next);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_is_null(next);

	/* remove previous request, so we should recover behavior given by
	 * first request.
	 */
	rv = pm_policy_latency_request_remove(&req2);
	zassert_equal(rv, 0);

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

	/* add new request (expected notification) */
	expected_latency = 10000;
	pm_policy_latency_request_add(&req1, 10000);


	expected_latency = 50000;
	pm_policy_latency_request_update(&req1, 50000);

	/* add a new request, with higher value (no notification, previous
	 * prevails)
	 */
	pm_policy_latency_request_add(&req2, 60000);

	pm_policy_latency_request_remove(&req2);

	/* remove first request, we no longer have latency requirements */
	expected_latency = SYS_FOREVER_US;
	pm_policy_latency_request_remove(&req1);
}

struct test_data {
	struct k_timer timer;
	struct onoff_manager mgr;
	struct onoff_monitor monitor;
	int res;
	onoff_notify_fn notify;
	k_timeout_t timeout;
	bool on;
};

struct test_req {
	struct pm_policy_latency_request req;
	volatile int result;
};

static void onoff_monitor_cb(struct onoff_manager *mgr,
			     struct onoff_monitor *mon,
			     uint32_t state,
			     int res)
{
	struct test_data *data = CONTAINER_OF(mgr, struct test_data, mgr);

	data->on = (state == ONOFF_STATE_ON);
}

static void onoff_timeout(struct k_timer *timer)
{
	struct test_data *data = CONTAINER_OF(timer, struct test_data, timer);

	data->notify(&data->mgr, data->res);
}

K_TIMER_DEFINE(onoff_timer, onoff_timeout, NULL);

static void onoff_start(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct test_data *data = CONTAINER_OF(mgr, struct test_data, mgr);

	data->notify = notify;
	k_timer_start(&data->timer, data->timeout, K_NO_WAIT);
}

static void onoff_stop(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct test_data *data = CONTAINER_OF(mgr, struct test_data, mgr);

	data->notify = notify;
	k_timer_start(&data->timer, data->timeout, K_NO_WAIT);
}

static void immediate_ctrl_binary_init(struct test_data *data, uint32_t thr, uint32_t t_op)
{
	static const struct onoff_transitions transitions = {
		.start = onoff_start,
		.stop = onoff_stop
	};
	static struct pm_policy_latency_immediate_binary bin_mgr;
	struct pm_policy_latency_immediate_ctrl ctrl;
	int rv;

	bin_mgr.mgr = &data->mgr;
	bin_mgr.thr = thr;
	ctrl.onoff = true;
	ctrl.bin_mgr = &bin_mgr;
	data->monitor.callback = onoff_monitor_cb;

	data->timeout = K_MSEC(t_op);
	k_timer_init(&data->timer, onoff_timeout, NULL);
	rv = onoff_manager_init(&data->mgr, &transitions);
	zassert_equal(rv, 0);

	rv = onoff_monitor_register(&data->mgr, &data->monitor);
	zassert_equal(rv, 0);

	rv = pm_policy_latency_immediate_ctrl_add(&ctrl);
	zassert_equal(rv, 0);
}

static void latency_changed(struct pm_policy_latency_request *req, int32_t result)
{
	struct test_req *test_req = CONTAINER_OF(req, struct test_req, req);

	test_req->result = result;
}

ZTEST(policy_api, test_pm_policy_latency_immediate_action_bin)
{
	struct test_data data;
	uint32_t thr = 1000;
	uint32_t t_op = 1;
	struct test_req req1, req2;
	int rv;
	int result = -EAGAIN;

	immediate_ctrl_binary_init(&data, thr, t_op);

	sys_notify_init_spinwait(&req1.req.cli.notify);

	/* Latency above threhold so requirement is already met. */
	rv = pm_policy_latency_request_add(&req1.req, thr + 1);
	zassert_equal(rv, 0, "Unexpected rv: %d", rv);
	zassert_equal(sys_notify_fetch_result(&req1.req.cli.notify, &result), 0);
	zassert_equal(result, 0, "Unexpected result: %d", result);

	req2.result = -EIO;
	sys_notify_init_callback(&req2.req.cli.notify, latency_changed);
	rv = pm_policy_latency_request_add(&req2.req, thr + 2);
	zassert_equal(rv, 0, "Unexpected rv: %d", rv);
	zassert_equal(req2.result, 0, "Unexpected rv: %d", req2.result);

	req2.result = -EIO;
	sys_notify_init_callback(&req2.req.cli.notify, latency_changed);
	/* Below the threshold, it triggers asynchronous action. */
	rv = pm_policy_latency_request_update(&req2.req, thr - 1);
	zassert_equal(rv, 1, "Unexpected rv: %d", rv);
	/* Not finished yet. */
	zassert_equal(req2.result, -EIO, "Unexpected rv: %d", req2.result);
	k_msleep(t_op);
	zassert_equal(req2.result, 0, "Unexpected rv: %d", req2.result);

	rv = pm_policy_latency_request_remove(&req1.req);
	zassert_equal(rv, 0, "Unexpected rv: %d", rv);

	rv = pm_policy_latency_request_remove(&req2.req);
	zassert_equal(rv, 0, "Unexpected rv: %d", rv);

	zassert_false(data.on);
	rv = pm_policy_latency_request_add_sync(&req1.req, thr - 1);
	zassert_equal(rv, 0, "Unexpected rv: %d", rv);
	/* Synchronous request blocks until immediate action is completed. */
	zassert_true(data.on);

	rv = pm_policy_latency_request_remove(&req1.req);
	zassert_equal(rv, 0, "Unexpected rv: %d", rv);

	/* Add request below the threshold. */
	rv = pm_policy_latency_request_add(&req1.req, thr + 1);
	zassert_equal(rv, 0, "Unexpected rv: %d", rv);
	zassert_false(data.on);

	/* Synchronous request for update that trigger the immediate action. */
	rv = pm_policy_latency_request_update_sync(&req1.req, thr - 1);
	zassert_equal(rv, 0, "Unexpected rv: %d", rv);
	zassert_true(data.on);

	rv = pm_policy_latency_request_remove(&req1.req);
	zassert_equal(rv, 0, "Unexpected rv: %d", rv);
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

ZTEST(policy_api, test_pm_policy_events)
{
	struct pm_policy_event evt1;
	struct pm_policy_event evt2;
	uint32_t now_cycle;
	uint32_t evt1_1_cycle;
	uint32_t evt1_2_cycle;
	uint32_t evt2_cycle;

	now_cycle = k_cycle_get_32();
	evt1_1_cycle = now_cycle + k_ticks_to_cyc_floor32(100);
	evt1_2_cycle = now_cycle + k_ticks_to_cyc_floor32(200);
	evt2_cycle = now_cycle + k_ticks_to_cyc_floor32(2000);

	zassert_equal(pm_policy_next_event_ticks(), -1);
	pm_policy_event_register(&evt1, evt1_1_cycle);
	pm_policy_event_register(&evt2, evt2_cycle);
	zassert_within(pm_policy_next_event_ticks(), 100, 50);
	pm_policy_event_unregister(&evt1);
	zassert_within(pm_policy_next_event_ticks(), 2000, 50);
	pm_policy_event_unregister(&evt2);
	zassert_equal(pm_policy_next_event_ticks(), -1);
	pm_policy_event_register(&evt2, evt2_cycle);
	zassert_within(pm_policy_next_event_ticks(), 2000, 50);
	pm_policy_event_register(&evt1, evt1_1_cycle);
	zassert_within(pm_policy_next_event_ticks(), 100, 50);
	pm_policy_event_update(&evt1, evt1_2_cycle);
	zassert_within(pm_policy_next_event_ticks(), 200, 50);
	pm_policy_event_unregister(&evt1);
	pm_policy_event_unregister(&evt2);
}

ZTEST_SUITE(policy_api, NULL, NULL, NULL, NULL, NULL);
