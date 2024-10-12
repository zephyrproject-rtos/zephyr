/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/net/prometheus/counter.h>

struct prometheus_metric test_counter_metric = {
	.type = PROMETHEUS_COUNTER,
	.name = "test_counter",
	.description = "Test counter",
	.num_labels = 1,
	.labels = {{
		.key = "test",
		.value = "counter",
	}},
};

PROMETHEUS_COUNTER_DEFINE(test_counter_m, &test_counter_metric);

/**
 * @brief Test prometheus_counter_inc
 * @details The test shall increment the counter value by 1 and check if the
 * value is incremented correctly.
 */
ZTEST(test_counter, test_prometheus_counter_inc)
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

ZTEST_SUITE(test_counter, NULL, NULL, NULL, NULL, NULL);
