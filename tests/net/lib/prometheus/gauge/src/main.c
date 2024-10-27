/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/net/prometheus/gauge.h>

struct prometheus_metric test_gauge_metric = {
	.type = PROMETHEUS_GAUGE,
	.name = "test_gauge",
	.description = "Test gauge",
	.num_labels = 1,
	.labels = {{
		.key = "test",
		.value = "gauge",
	}},
};

PROMETHEUS_GAUGE_DEFINE(test_gauge_m, &test_gauge_metric);

/**
 * @brief Test prometheus_gauge_set
 *
 * @details The test shall set the gauge value to 1 and check if the
 * value is set correctly.
 *
 * @details The test shall set the gauge value to 2 and check if the
 * value is set correctly.
 */
ZTEST(test_gauge, test_gauge_set)
{
	int ret;

	zassert_equal(test_gauge_m.value, 0, "Gauge value is not 0");

	ret = prometheus_gauge_set(&test_gauge_m, 1);
	zassert_ok(ret, "Error setting gauge");

	zassert_equal(test_gauge_m.value, 1, "Gauge value is not 1");

	ret = prometheus_gauge_set(&test_gauge_m, 2);
	zassert_ok(ret, "Error setting gauge");

	zassert_equal(test_gauge_m.value, 2, "Gauge value is not 2");
}

ZTEST_SUITE(test_gauge, NULL, NULL, NULL, NULL, NULL);
