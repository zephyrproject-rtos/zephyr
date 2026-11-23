/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/net/prometheus/counter.h>

PROMETHEUS_COUNTER_DEFINE(test_counter_m, "Test counter",
			  ({ .key = "test_counter", .value = "test" }),
			  NULL);

/**
 * @brief Test prometheus_counter_inc
 * @details The test shall increment the counter value by 1 and check if the
 * value is incremented correctly.
 */
ZTEST(test_counter, test_prometheus_counter_01_inc)
{
	int ret;

	zassert_equal(test_counter_m.value, 0, "Counter value is not 0");

	ret = prometheus_counter_inc(&test_counter_m);
	zassert_ok(ret, "Error incrementing counter");

	zassert_equal(test_counter_m.value, 1, "Counter value is not 1");

	ret = prometheus_counter_inc(&test_counter_m);
	zassert_ok(ret, "Error incrementing counter");

	zassert_equal(test_counter_m.value, 2, "Counter value is not 2");
}

/**
 * @brief Test prometheus_counter_add
 * @details The test shall increment the counter value by arbitrary value
 * and check if the value is incremented correctly.
 */
ZTEST(test_counter, test_prometheus_counter_02_add)
{
	int ret;

	ret = prometheus_counter_add(&test_counter_m, 2);
	zassert_ok(ret, "Error adding counter");

	zassert_equal(test_counter_m.value, 4, "Counter value is not 4");

	ret = prometheus_counter_add(&test_counter_m, 0);
	zassert_ok(ret, "Error adding counter");

	zassert_equal(test_counter_m.value, 4, "Counter value is not 4");
}

/**
 * @brief Test prometheus_counter_set
 * @details The test shall set the counter value by arbitrary value
 * and check if the value is incremented correctly.
 */
ZTEST(test_counter, test_prometheus_counter_03_set)
{
	int ret;

	ret = prometheus_counter_set(&test_counter_m, 20);
	zassert_ok(ret, "Error setting counter");

	zassert_equal(test_counter_m.value, 20, "Counter value is not 20");

	ret = prometheus_counter_set(&test_counter_m, 15);
	zassert_equal(ret, -EINVAL, "Error setting counter");

	zassert_equal(test_counter_m.value, 20, "Counter value is not 20");
}

ZTEST_SUITE(test_counter, NULL, NULL, NULL, NULL, NULL);
