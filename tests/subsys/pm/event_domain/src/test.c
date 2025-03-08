/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/event_domain.h>

#define TEST_EVENT_DEVICE_0 DT_NODELABEL(test_event_device0)
#define TEST_EVENT_DEVICE_1 DT_NODELABEL(test_event_device1)
#define TEST_EVENT_DOMAIN   DT_NODELABEL(test_event_domain0)

#define TEST_EVENT_DOMAIN_LATENCY_US_0 \
	DT_EVENT_DOMAIN_EVENT_LATENCY_US_BY_IDX(TEST_EVENT_DOMAIN, 0)

#define TEST_EVENT_DOMAIN_LATENCY_US_1 \
	DT_EVENT_DOMAIN_EVENT_LATENCY_US_BY_IDX(TEST_EVENT_DOMAIN, 1)

#define TEST_EVENT_DOMAIN_LATENCY_US_2 \
	DT_EVENT_DOMAIN_EVENT_LATENCY_US_BY_IDX(TEST_EVENT_DOMAIN, 2)

#define TEST_EVENT_DOMAIN_DEVICE_STATE_0_0 \
	DT_EVENT_DOMAIN_EVENT_DEVICE_STATE_BY_IDX(TEST_EVENT_DOMAIN, 0)

#define TEST_EVENT_DOMAIN_DEVICE_STATE_0_1 \
	DT_EVENT_DOMAIN_EVENT_DEVICE_STATE_BY_IDX(TEST_EVENT_DOMAIN, 1)

#define TEST_EVENT_DOMAIN_DEVICE_STATE_1_0 \
	DT_EVENT_DOMAIN_EVENT_DEVICE_STATE_BY_IDX(TEST_EVENT_DOMAIN, 2)

#define TEST_EVENT_DOMAIN_DEVICE_STATE_1_1 \
	DT_EVENT_DOMAIN_EVENT_DEVICE_STATE_BY_IDX(TEST_EVENT_DOMAIN, 3)

#define TEST_EVENT_DOMAIN_DEVICE_STATE_2_0 \
	DT_EVENT_DOMAIN_EVENT_DEVICE_STATE_BY_IDX(TEST_EVENT_DOMAIN, 4)

#define TEST_EVENT_DOMAIN_DEVICE_STATE_2_1 \
	DT_EVENT_DOMAIN_EVENT_DEVICE_STATE_BY_IDX(TEST_EVENT_DOMAIN, 5)

#define TEST_EVENT_DEVICE_EVENT_STATES_COUNT             3
#define TEST_EVENT_DEVICE_EVENT_STATE_REQUEST_LATENCY_US 100

#define TEST_TIMEOUT K_SECONDS(1)

struct test_event_device_data {
	uint8_t event_state;
	struct k_sem event_state_sem;
};

struct test_event_device_config {
	const struct pm_event_device *event_device;
};

static void test_event_domain_event_device_request(const struct device *dev, uint8_t event_state)
{
	struct test_event_device_data *data = dev->data;

	data->event_state = event_state;
	k_sem_give(&data->event_state_sem);
}

static int test_event_domain_event_device_init(const struct device *dev)
{
	struct test_event_device_data *data = dev->data;
	const struct test_event_device_config *config = dev->config;

	k_sem_init(&data->event_state_sem, 0, 1);
	pm_event_device_init(config->event_device);
	return k_sem_take(&data->event_state_sem, K_NO_WAIT);
}

static int test_event_domain_event_device_wait_for_request(const struct device *dev)
{
	struct test_event_device_data *data = dev->data;

	if (k_sem_take(&data->event_state_sem, TEST_TIMEOUT)) {
		return -ETIMEDOUT;
	}

	return data->event_state;
}

struct test_event_device_data test_event_device_data0;
const struct test_event_device_config test_event_device_config0 = {
	.event_device = PM_EVENT_DEVICE_DT_GET(TEST_EVENT_DEVICE_0),
};

DEVICE_DT_DEFINE(
	TEST_EVENT_DEVICE_0,
	test_event_domain_event_device_init,
	NULL,
	&test_event_device_data0,
	&test_event_device_config0,
	POST_KERNEL,
	99,
	NULL
);

PM_EVENT_DEVICE_DT_DEFINE(
	TEST_EVENT_DEVICE_0,
	test_event_domain_event_device_request,
	TEST_EVENT_DEVICE_EVENT_STATE_REQUEST_LATENCY_US,
	TEST_EVENT_DEVICE_EVENT_STATES_COUNT
);

struct test_event_device_data test_event_device_data1;
const struct test_event_device_config test_event_device_config1 = {
	.event_device = PM_EVENT_DEVICE_DT_GET(TEST_EVENT_DEVICE_1),
};

DEVICE_DT_DEFINE(
	TEST_EVENT_DEVICE_1,
	test_event_domain_event_device_init,
	NULL,
	&test_event_device_data1,
	&test_event_device_config1,
	POST_KERNEL,
	99,
	NULL
);

PM_EVENT_DEVICE_DT_DEFINE(
	TEST_EVENT_DEVICE_1,
	test_event_domain_event_device_request,
	TEST_EVENT_DEVICE_EVENT_STATE_REQUEST_LATENCY_US,
	TEST_EVENT_DEVICE_EVENT_STATES_COUNT
);

static const struct device *test_dev0 = DEVICE_DT_GET(TEST_EVENT_DEVICE_0);
static const struct device *test_dev1 = DEVICE_DT_GET(TEST_EVENT_DEVICE_1);
static const struct pm_event_domain *test_event_domain = PM_EVENT_DOMAIN_DT_GET(TEST_EVENT_DOMAIN);
PM_EVENT_DOMAIN_EVENT_DT_DEFINE(test_event0, TEST_EVENT_DOMAIN);

ZTEST(pm_event_domain, test_request_release)
{
	int ret;

	pm_event_domain_request_event(&test_event0, TEST_EVENT_DOMAIN_LATENCY_US_0);
	ret = test_event_domain_event_device_wait_for_request(test_dev0);
	zassert_equal(ret, -ETIMEDOUT);
	ret = test_event_domain_event_device_wait_for_request(test_dev1);
	zassert_equal(ret, -ETIMEDOUT);

	pm_event_domain_release_event(&test_event0);
	ret = test_event_domain_event_device_wait_for_request(test_dev0);
	zassert_equal(ret, -ETIMEDOUT);
	ret = test_event_domain_event_device_wait_for_request(test_dev1);
	zassert_equal(ret, -ETIMEDOUT);

	pm_event_domain_request_event(&test_event0, TEST_EVENT_DOMAIN_LATENCY_US_1);
	ret = test_event_domain_event_device_wait_for_request(test_dev0);
	zassert_equal(ret, TEST_EVENT_DOMAIN_DEVICE_STATE_1_0);
	ret = test_event_domain_event_device_wait_for_request(test_dev1);
	zassert_equal(ret, TEST_EVENT_DOMAIN_DEVICE_STATE_1_1);

	pm_event_domain_release_event(&test_event0);
	ret = test_event_domain_event_device_wait_for_request(test_dev0);
	zassert_equal(ret, TEST_EVENT_DOMAIN_DEVICE_STATE_0_0);
	ret = test_event_domain_event_device_wait_for_request(test_dev1);
	zassert_equal(ret, TEST_EVENT_DOMAIN_DEVICE_STATE_0_1);

	pm_event_domain_request_event(&test_event0, TEST_EVENT_DOMAIN_LATENCY_US_2);
	ret = test_event_domain_event_device_wait_for_request(test_dev0);
	zassert_equal(ret, TEST_EVENT_DOMAIN_DEVICE_STATE_2_0);
	ret = test_event_domain_event_device_wait_for_request(test_dev1);
	zassert_equal(ret, TEST_EVENT_DOMAIN_DEVICE_STATE_2_1);
}

ZTEST(pm_event_domain, test_floor_event_latency_us)
{
	uint32_t latency_us;

	latency_us = pm_event_domain_floor_event_latency_us(test_event_domain,
							    TEST_EVENT_DOMAIN_LATENCY_US_0);
	zassert_equal(latency_us, TEST_EVENT_DOMAIN_LATENCY_US_0);
	latency_us = pm_event_domain_floor_event_latency_us(test_event_domain,
							    TEST_EVENT_DOMAIN_LATENCY_US_0 + 1);
	zassert_equal(latency_us, TEST_EVENT_DOMAIN_LATENCY_US_0);
	latency_us = pm_event_domain_floor_event_latency_us(test_event_domain,
							    TEST_EVENT_DOMAIN_LATENCY_US_0 - 1);
	zassert_equal(latency_us, TEST_EVENT_DOMAIN_LATENCY_US_1);

	latency_us = pm_event_domain_floor_event_latency_us(test_event_domain,
							    TEST_EVENT_DOMAIN_LATENCY_US_1);
	zassert_equal(latency_us, TEST_EVENT_DOMAIN_LATENCY_US_1);
	latency_us = pm_event_domain_floor_event_latency_us(test_event_domain,
							    TEST_EVENT_DOMAIN_LATENCY_US_1 + 1);
	zassert_equal(latency_us, TEST_EVENT_DOMAIN_LATENCY_US_1);
	latency_us = pm_event_domain_floor_event_latency_us(test_event_domain,
							    TEST_EVENT_DOMAIN_LATENCY_US_1 - 1);
	zassert_equal(latency_us, TEST_EVENT_DOMAIN_LATENCY_US_2);

	latency_us = pm_event_domain_floor_event_latency_us(test_event_domain,
							    TEST_EVENT_DOMAIN_LATENCY_US_2);
	zassert_equal(latency_us, TEST_EVENT_DOMAIN_LATENCY_US_2);
	latency_us = pm_event_domain_floor_event_latency_us(test_event_domain,
							    TEST_EVENT_DOMAIN_LATENCY_US_2 + 1);
	zassert_equal(latency_us, TEST_EVENT_DOMAIN_LATENCY_US_2);
	latency_us = pm_event_domain_floor_event_latency_us(test_event_domain,
							    TEST_EVENT_DOMAIN_LATENCY_US_2 - 1);
	zassert_equal(latency_us, TEST_EVENT_DOMAIN_LATENCY_US_2);
}

ZTEST_SUITE(pm_event_domain, NULL, NULL, NULL, NULL, NULL);
