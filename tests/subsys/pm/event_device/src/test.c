/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/event_device.h>

#define TEST_NODE \
	DT_NODELABEL(test_event_device)

#define TEST_DEVICE \
	DEVICE_DT_GET(TEST_NODE)

#define TEST_EVENT_STATE_REQUEST_LATENCY_US 1000
#define TEST_EVENT_STATE_COUNT              4

#define TEST_EVENT_STATE_REQUEST_LATENCY_TICKS \
	k_us_to_ticks_ceil32(TEST_EVENT_STATE_REQUEST_LATENCY_US)

#define TEST_MAX_REQUEST_TIMEOUT \
	K_TICKS(TEST_EVENT_STATE_REQUEST_LATENCY_TICKS * 2 + 1)

#define TEST_EVENT_INTERVAL_TICKS \
	(TEST_EVENT_STATE_REQUEST_LATENCY_TICKS * 4 + 1)

DEVICE_DT_DEFINE(
	TEST_NODE,
	NULL,
	NULL,
	NULL,
	NULL,
	POST_KERNEL,
	99,
	NULL
);

static const struct device *test_event_device_dev = TEST_DEVICE;
static int64_t test_request_mock_uptime_ticks;
static uint8_t test_request_mock_event_state;
static struct k_spinlock test_request_mock_lock;
static bool test_request_mock_waiting;
static K_SEM_DEFINE(test_request_mock_sem, 0, 1);

static void test_event_device_dev_request(const struct device *dev, uint8_t event_state)
{
	K_SPINLOCK(&test_request_mock_lock) {
		zassert_true(test_request_mock_waiting);
		test_request_mock_waiting = false;
		zassert_equal(k_uptime_ticks(), test_request_mock_uptime_ticks);
		zassert_equal(dev, test_event_device_dev);
		zassert_equal(event_state, test_request_mock_event_state);
	}

	k_sem_give(&test_request_mock_sem);
}

PM_EVENT_DEVICE_DT_DEFINE(
	TEST_NODE,
	test_event_device_dev_request,
	TEST_EVENT_STATE_REQUEST_LATENCY_US,
	TEST_EVENT_STATE_COUNT
);

const struct pm_event_device *test_event_device = PM_EVENT_DEVICE_DT_GET(TEST_NODE);

static void test_event_device_request_mock_expect_call(int64_t uptime_ticks, uint8_t event_state)
{
	K_SPINLOCK(&test_request_mock_lock) {
		zassert_false(test_request_mock_waiting);
		test_request_mock_waiting = true;
		test_request_mock_uptime_ticks = uptime_ticks;
		test_request_mock_event_state = event_state;
	}
}

static void test_event_device_request_mock_await_call(void)
{
	(void)k_sem_take(&test_request_mock_sem, K_FOREVER);
}

static void test_event_device_request_mock_await_no_call(void)
{
	k_sleep(TEST_MAX_REQUEST_TIMEOUT);
}

static void *test_setup(void)
{
	test_event_device_request_mock_expect_call(k_uptime_ticks(), 0);
	pm_event_device_init(test_event_device);
	test_event_device_request_mock_await_call();
	return NULL;
}

static void test_before(void *f)
{
	test_event_device_request_mock_await_no_call();
}

ZTEST(pm_event_device, test_request_release)
{
	struct pm_event_device_event event0;
	struct pm_event_device_event event1;
	struct pm_event_device_event event2;
	struct pm_event_device_event event3;
	int64_t uptime_ticks;
	int64_t effective_uptime_ticks;

	/* Request highest latency which shall have no effect */
	uptime_ticks = k_uptime_ticks();
	effective_uptime_ticks = pm_event_device_request_event(test_event_device, &event0, 0);
	zassert_equal(effective_uptime_ticks, uptime_ticks);
	test_event_device_request_mock_await_no_call();

	/* Request second highest latency which shall result in immediate call to EVENT_DOMAIN */
	uptime_ticks = k_uptime_ticks();
	test_event_device_request_mock_expect_call(uptime_ticks, 1);
	effective_uptime_ticks = pm_event_device_request_event(test_event_device, &event1, 1);
	zassert_equal(effective_uptime_ticks,
		      uptime_ticks + TEST_EVENT_STATE_REQUEST_LATENCY_TICKS + 1);
	test_event_device_request_mock_await_call();

	/*
	 * Request third highest latency which shall result in call to EVENT_DOMAIN after
	 * previous request is in effect.
	 */
	uptime_ticks = effective_uptime_ticks;
	test_event_device_request_mock_expect_call(uptime_ticks, 2);
	effective_uptime_ticks = pm_event_device_request_event(test_event_device, &event2, 2);
	zassert_equal(effective_uptime_ticks,
		      uptime_ticks + TEST_EVENT_STATE_REQUEST_LATENCY_TICKS + 1);
	test_event_device_request_mock_await_call();

	/*
	 * Request lowest latency which shall result in call to EVENT_DOMAIN after previous
	 * request is in effect.
	 */
	uptime_ticks = effective_uptime_ticks;
	test_event_device_request_mock_expect_call(uptime_ticks, 3);
	effective_uptime_ticks = pm_event_device_request_event(test_event_device, &event3, 3);
	zassert_equal(effective_uptime_ticks,
		      uptime_ticks + TEST_EVENT_STATE_REQUEST_LATENCY_TICKS + 1);
	test_event_device_request_mock_await_call();

	/*
	 * Release second highest latency which shall have no effect given we have an
	 * active lower latency request.
	 */
	pm_event_device_release_event(&event1);
	test_event_device_request_mock_await_no_call();

	/*
	 * Release lowest latency which shall have immediate effect given we waitied
	 * long enough to make sure any requested latency is in effect with
	 * test_event_device_request_mock_await_no_call().
	 */
	uptime_ticks = k_uptime_ticks();
	test_event_device_request_mock_expect_call(uptime_ticks, 2);
	pm_event_device_release_event(&event3);
	test_event_device_request_mock_await_call();

	/*
	 * Release third highest latency which shall result in call to EVENT_DOMAIN after
	 * previous request is in effect. We need to calculate this time manually
	 * since pm_event_device_release_event() does not provide this time.
	 */
	uptime_ticks += TEST_EVENT_STATE_REQUEST_LATENCY_TICKS + 1;
	test_event_device_request_mock_expect_call(uptime_ticks, 0);
	pm_event_device_release_event(&event2);
	test_event_device_request_mock_await_call();

	/* Release highest latency which shall have no effect */
	pm_event_device_release_event(&event0);
	test_event_device_request_mock_await_no_call();
}

ZTEST(pm_event_device, test_schedule_release)
{
	struct pm_event_device_event event0;
	int64_t event_uptime_ticks;
	int64_t uptime_ticks;
	int64_t effective_uptime_ticks;

	event_uptime_ticks = k_uptime_ticks() + TEST_EVENT_INTERVAL_TICKS;
	uptime_ticks = event_uptime_ticks - TEST_EVENT_STATE_REQUEST_LATENCY_TICKS - 1;
	test_event_device_request_mock_expect_call(uptime_ticks, 1);
	effective_uptime_ticks = pm_event_device_schedule_event(test_event_device,
								&event0,
								1,
								event_uptime_ticks);
	zassert_equal(effective_uptime_ticks, event_uptime_ticks);
	test_event_device_request_mock_await_call();

	test_event_device_request_mock_expect_call(effective_uptime_ticks, 0);
	pm_event_device_release_event(&event0);
	test_event_device_request_mock_await_call();
}

ZTEST(pm_event_device, test_schedule_release_overlap)
{
	struct pm_event_device_event event0;
	struct pm_event_device_event event1;
	int64_t event0_uptime_ticks;
	int64_t event1_uptime_ticks;
	int64_t uptime_ticks;
	int64_t effective_uptime_ticks;

	event0_uptime_ticks = k_uptime_ticks() + TEST_EVENT_INTERVAL_TICKS;
	event1_uptime_ticks = event0_uptime_ticks + 1;

	uptime_ticks = event0_uptime_ticks - TEST_EVENT_STATE_REQUEST_LATENCY_TICKS - 1;
	test_event_device_request_mock_expect_call(uptime_ticks, 2);

	effective_uptime_ticks = pm_event_device_schedule_event(test_event_device,
								&event0,
								1,
								event0_uptime_ticks);
	zassert_equal(effective_uptime_ticks, event0_uptime_ticks);
	effective_uptime_ticks = pm_event_device_schedule_event(test_event_device,
								&event1,
								2,
								event1_uptime_ticks);
	zassert_equal(effective_uptime_ticks, event1_uptime_ticks);

	test_event_device_request_mock_await_call();

	pm_event_device_release_event(&event0);
	test_event_device_request_mock_await_no_call();

	uptime_ticks = k_uptime_ticks();
	test_event_device_request_mock_expect_call(uptime_ticks, 0);
	pm_event_device_release_event(&event1);
	test_event_device_request_mock_await_call();
}

ZTEST_SUITE(pm_event_device, NULL, test_setup, test_before, NULL, NULL);
