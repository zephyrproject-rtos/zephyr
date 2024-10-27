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
	char formatted[MAX_BUFFER_SIZE];
	struct prometheus_counter *counter;
	char exposed[] = "# HELP test_counter Test counter\n"
			 "# TYPE test_counter counter\n"
			 "test_counter{test=\"counter\"} 1\n";

	prometheus_collector_register_metric(&test_custom_collector, test_counter_m.base);

	counter = (struct prometheus_counter *)prometheus_collector_get_metric(
		&test_custom_collector, "test_counter");

	zassert_equal(counter, &test_counter_m, "Counter not found in collector");

	zassert_equal(test_counter_m.value, 0, "Counter value is not 0");

	ret = prometheus_counter_inc(&test_counter_m);
	zassert_ok(ret, "Error incrementing counter");

	zassert_equal(counter->value, 1, "Counter value is not 1");

	ret = prometheus_format_exposition(&test_custom_collector, formatted, sizeof(formatted));
	zassert_ok(ret, "Error formatting exposition data");

	zassert_equal(strcmp(formatted, exposed), 0, "Exposition format is not as expected");
}

ZTEST_SUITE(test_formatter, NULL, NULL, NULL, NULL, NULL);
