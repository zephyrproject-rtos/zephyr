/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/net/prometheus/counter.h>
#include <zephyr/net/prometheus/collector.h>
#include <zephyr/net/prometheus/formatter.h>

#define MAX_BUFFER_SIZE 256

PROMETHEUS_COUNTER_DEFINE(test_counter, "Test counter",
			  ({ .key = "test", .value = "counter" }), NULL);
PROMETHEUS_COUNTER_DEFINE(test_counter2, "Test counter 2",
			  ({ .key = "test", .value = "counter" }), NULL);

PROMETHEUS_COLLECTOR_DEFINE(test_custom_collector);

/**
 * @brief Test Prometheus formatter
 * @details The test shall increment the counter value by 1 and check if the
 * value is incremented correctly. It shall then format the metric and compare
 * it with the expected output.
 */
ZTEST(test_formatter, test_prometheus_formatter_simple)
{
	int ret;
	char formatted[MAX_BUFFER_SIZE] = { 0 };
	struct prometheus_counter *counter;
	char exposed[] = "# HELP test_counter2 Test counter 2\n"
			 "# TYPE test_counter2 counter\n"
			 "test_counter2{test=\"counter\"} 1\n"
			 "# HELP test_counter Test counter\n"
			 "# TYPE test_counter counter\n"
			 "test_counter{test=\"counter\"} 1\n";

	prometheus_collector_register_metric(&test_custom_collector, &test_counter.base);
	prometheus_collector_register_metric(&test_custom_collector, &test_counter2.base);

	counter = (struct prometheus_counter *)prometheus_collector_get_metric(
		&test_custom_collector, "test_counter");

	zassert_equal(counter, &test_counter, "Counter not found in collector");

	zassert_equal(test_counter.value, 0, "Counter value is not 0");

	ret = prometheus_counter_inc(&test_counter);
	zassert_ok(ret, "Error incrementing counter");

	ret = prometheus_counter_inc(&test_counter2);
	zassert_ok(ret, "Error incrementing counter 2");

	zassert_equal(counter->value, 1, "Counter value is not 1");

	ret = prometheus_format_exposition(&test_custom_collector, formatted, sizeof(formatted));
	zassert_ok(ret, "Error formatting exposition data");

	zassert_equal(strcmp(formatted, exposed), 0,
		      "Exposition format is not as expected (expected\n\"%s\", got\n\"%s\")",
		      exposed, formatted);
}

ZTEST_SUITE(test_formatter, NULL, NULL, NULL, NULL, NULL);
