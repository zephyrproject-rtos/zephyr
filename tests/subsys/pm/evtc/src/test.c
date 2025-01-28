/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/evtc.h>

#define TEST_EVTC_NODE                DT_NODELABEL(evtc)
#define TEST_EVTC_DEVICE              DEVICE_DT_GET(TEST_EVTC_NODE)
#define TEST_EVTC_REQ_LAT_TKS         PM_EVTC_DT_REQ_LAT_TICKS(TEST_EVTC_NODE)
#define TEST_EVTC_LAT_NS_BY_IDX(_idx) PM_EVTC_DT_LAT_NS_BY_IDX(TEST_EVTC_NODE, _idx)
#define TEST_EVTC_LAT_NS_0            TEST_EVTC_LAT_NS_BY_IDX(0)
#define TEST_EVTC_LAT_NS_1            TEST_EVTC_LAT_NS_BY_IDX(1)
#define TEST_EVTC_LAT_NS_2            TEST_EVTC_LAT_NS_BY_IDX(2)
#define TEST_EVTC_LAT_NS_3            TEST_EVTC_LAT_NS_BY_IDX(3)
#define TEST_EVTC_LAT_NS_MAX          PM_EVTC_DT_LAT_NS_MAX(TEST_EVTC_NODE)
#define TEST_EVTC_LAT_NS_MIN          PM_EVTC_DT_LAT_NS_MIN(TEST_EVTC_NODE)

/* The max time from last evt released to evtc ready for new request */
#define TEST_EVTC_MAX_REQUEST_TIMEOUT K_TICKS(TEST_EVTC_REQ_LAT_TKS * 2 + 1)

#define TEST_EVTC_EVT_INTERVAL_TICKS (TEST_EVTC_REQ_LAT_TKS * 4 + 1)
#define TEST_EVTC_MAX_SCHEDULE_TIMEOUT

DEVICE_DT_DEFINE(
	DT_NODELABEL(evtc),
	NULL,
	NULL,
	NULL,
	NULL,
	POST_KERNEL,
	99,
	NULL
);

static const struct device *test_evtc_dev = TEST_EVTC_DEVICE;
static int64_t test_request_mock_uptime_ticks;
static uint32_t test_request_mock_latency_ns;
static struct k_spinlock test_request_mock_lock;
static bool test_request_mock_waiting;
static K_SEM_DEFINE(test_request_mock_sem, 0, 1);

static void test_evtc_dev_request(const struct device *dev, uint32_t latency_ns)
{
	K_SPINLOCK(&test_request_mock_lock) {
		zassert_true(test_request_mock_waiting);
		test_request_mock_waiting = false;
		zassert_equal(k_uptime_ticks(), test_request_mock_uptime_ticks);
		zassert_equal(dev, test_evtc_dev);
		zassert_equal(latency_ns, test_request_mock_latency_ns);
	}

	k_sem_give(&test_request_mock_sem);
}

PM_EVTC_DT_DEFINE(TEST_EVTC_NODE, test_evtc_dev_request);

const struct pm_evtc *test_evtc = PM_EVTC_DT_GET(TEST_EVTC_NODE);

static void test_evtc_request_mock_expect_call(int64_t uptime_ticks, uint32_t latency_ns)
{
	K_SPINLOCK(&test_request_mock_lock) {
		zassert_false(test_request_mock_waiting);
		test_request_mock_waiting = true;
		test_request_mock_uptime_ticks = uptime_ticks;
		test_request_mock_latency_ns = latency_ns;
	}
}

static void test_evtc_request_mock_await_call(void)
{
	(void)k_sem_take(&test_request_mock_sem, K_FOREVER);
}

static void test_evtc_request_mock_await_no_call(void)
{
	k_sleep(TEST_EVTC_MAX_REQUEST_TIMEOUT);
}

static void *test_setup(void)
{
	test_evtc_request_mock_expect_call(k_uptime_ticks(), TEST_EVTC_LAT_NS_MAX);
	pm_evtc_init(test_evtc);
	test_evtc_request_mock_await_call();
	return NULL;
}

static void test_before(void *f)
{
	test_evtc_request_mock_await_no_call();
}

ZTEST(pm_evtc, test_request_release)
{
	struct pm_evt evt0;
	struct pm_evt evt1;
	struct pm_evt evt2;
	struct pm_evt evt3;
	int64_t uptks;
	int64_t eff_uptks;

	/* Request highest latency which shall have no effect */
	uptks = k_uptime_ticks();
	eff_uptks = pm_evtc_request_evt(test_evtc, &evt0, TEST_EVTC_LAT_NS_0);
	zassert_equal(eff_uptks, uptks);
	test_evtc_request_mock_await_no_call();

	/* Request second highest latency which shall result in immediate call to EVTC */
	uptks = k_uptime_ticks();
	test_evtc_request_mock_expect_call(uptks, TEST_EVTC_LAT_NS_1);
	eff_uptks = pm_evtc_request_evt(test_evtc, &evt1, TEST_EVTC_LAT_NS_1);
	zassert_equal(eff_uptks, uptks + TEST_EVTC_REQ_LAT_TKS + 1);
	test_evtc_request_mock_await_call();

	/*
	 * Request third highest latency which shall result in call to EVTC after
	 * previous request is in effect.
	 */
	uptks = eff_uptks;
	test_evtc_request_mock_expect_call(uptks, TEST_EVTC_LAT_NS_2);
	eff_uptks = pm_evtc_request_evt(test_evtc, &evt2, TEST_EVTC_LAT_NS_2);
	zassert_equal(eff_uptks, uptks + TEST_EVTC_REQ_LAT_TKS + 1);
	test_evtc_request_mock_await_call();

	/*
	 * Request lowest latency which shall result in call to EVTC after previous
	 * request is in effect.
	 */
	uptks = eff_uptks;
	test_evtc_request_mock_expect_call(uptks, TEST_EVTC_LAT_NS_3);
	eff_uptks = pm_evtc_request_evt(test_evtc, &evt3, TEST_EVTC_LAT_NS_3);
	zassert_equal(eff_uptks, uptks + TEST_EVTC_REQ_LAT_TKS + 1);
	test_evtc_request_mock_await_call();

	/*
	 * Release second highest latency which shall have no effect given we have an
	 * active lower latency request.
	 */
	pm_evtc_release_evt(&evt1);
	test_evtc_request_mock_await_no_call();

	/*
	 * Release lowest latency which shall have immediate effect given we waitied
	 * long enough to make sure any requested latency is in effect with
	 * test_evtc_request_mock_await_no_call().
	 */
	uptks = k_uptime_ticks();
	test_evtc_request_mock_expect_call(uptks, TEST_EVTC_LAT_NS_2);
	pm_evtc_release_evt(&evt3);
	test_evtc_request_mock_await_call();

	/*
	 * Release third highest latency which shall result in call to EVTC after
	 * previous request is in effect. We need to calculate this time manually
	 * since pm_evtc_release_evt() does not provide this time.
	 */
	uptks += TEST_EVTC_REQ_LAT_TKS + 1;
	test_evtc_request_mock_expect_call(uptks, TEST_EVTC_LAT_NS_0);
	pm_evtc_release_evt(&evt2);
	test_evtc_request_mock_await_call();

	/* Release highest latency which shall have no effect */
	pm_evtc_release_evt(&evt0);
	test_evtc_request_mock_await_no_call();
}

ZTEST(pm_evtc, test_schedule_release)
{
	struct pm_evt evt0;
	int64_t evt_uptks;
	int64_t uptks;
	int64_t eff_uptks;

	evt_uptks = k_uptime_ticks() + TEST_EVTC_EVT_INTERVAL_TICKS;
	uptks = evt_uptks - TEST_EVTC_REQ_LAT_TKS - 1;
	test_evtc_request_mock_expect_call(uptks, TEST_EVTC_LAT_NS_1);
	eff_uptks = pm_evtc_schedule_evt(test_evtc, &evt0, TEST_EVTC_LAT_NS_1, evt_uptks);
	zassert_equal(eff_uptks, evt_uptks);
	test_evtc_request_mock_await_call();

	test_evtc_request_mock_expect_call(eff_uptks, TEST_EVTC_LAT_NS_0);
	pm_evtc_release_evt(&evt0);
	test_evtc_request_mock_await_call();
}

ZTEST(pm_evtc, test_schedule_release_overlap)
{
	struct pm_evt evt0;
	struct pm_evt evt1;
	int64_t evt0_uptks;
	int64_t evt1_uptks;
	int64_t uptks;
	int64_t eff_uptks;

	evt0_uptks = k_uptime_ticks() + TEST_EVTC_EVT_INTERVAL_TICKS;
	evt1_uptks = evt0_uptks + 1;

	uptks = evt0_uptks - TEST_EVTC_REQ_LAT_TKS - 1;
	test_evtc_request_mock_expect_call(uptks, TEST_EVTC_LAT_NS_2);

	eff_uptks = pm_evtc_schedule_evt(test_evtc, &evt0, TEST_EVTC_LAT_NS_1, evt0_uptks);
	zassert_equal(eff_uptks, evt0_uptks);
	eff_uptks = pm_evtc_schedule_evt(test_evtc, &evt1, TEST_EVTC_LAT_NS_2, evt1_uptks);
	zassert_equal(eff_uptks, evt1_uptks);

	test_evtc_request_mock_await_call();

	pm_evtc_release_evt(&evt0);
	test_evtc_request_mock_await_no_call();

	uptks = k_uptime_ticks();
	test_evtc_request_mock_expect_call(uptks, TEST_EVTC_LAT_NS_0);
	pm_evtc_release_evt(&evt1);
	test_evtc_request_mock_await_call();
}

ZTEST_SUITE(pm_evtc, NULL, test_setup, test_before, NULL, NULL);
