/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/net/prometheus/counter.h>
#include <zephyr/net/prometheus/collector.h>

PROMETHEUS_COUNTER_DEFINE(test_counter_m, "Test counter",
			  ({ .key = "test_counter", .value = "test" }), NULL);

PROMETHEUS_COLLECTOR_DEFINE(test_custom_collector);

/**
 * @brief Test prometheus_counter_inc
 *
 * @details The test shall increment the counter value by 1 and check if the
 * value is incremented correctly.
 *
 * @details The test shall register the counter to the collector and check if the
 * counter is found in the collector.
 */
ZTEST(test_collector, test_prometheus_collector_register)
{
	int ret;
	struct prometheus_counter *counter;

	prometheus_collector_register_metric(&test_custom_collector, &test_counter_m.base);

	counter = (struct prometheus_counter *)prometheus_collector_get_metric(
		&test_custom_collector, "test_counter_m");

	zassert_equal_ptr(counter, &test_counter_m,
			  "Counter not found in collector (expected %p, got %p)",
			  &test_counter_m, counter);

	zassert_equal(test_counter_m.value, 0, "Counter value is not 0");

	ret = prometheus_counter_inc(counter);
	zassert_ok(ret, "Error incrementing counter");

	zassert_equal(counter->value, 1, "Counter value is not 1");
}

ZTEST_SUITE(test_collector, NULL, NULL, NULL, NULL, NULL);
