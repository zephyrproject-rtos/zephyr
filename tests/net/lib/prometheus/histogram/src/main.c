/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/net/prometheus/histogram.h>

struct prometheus_metric test_histogram_metric = {
	.type = PROMETHEUS_HISTOGRAM,
	.name = "test_histogram",
	.description = "Test histogram",
	.num_labels = 1,
	.labels = {{
		.key = "test",
		.value = "histogram",
	}},
};

PROMETHEUS_HISTOGRAM_DEFINE(test_histogram_m, &test_histogram_metric);

/**
 * @brief Test prometheus_histogram_observe
 *
 * @details The test shall observe the histogram value by 1 and check if the
 * value is incremented correctly.
 *
 * @details The test shall observe the histogram value by 2 and check if the
 * value is incremented correctly.
 */
ZTEST(test_histogram, test_histogram_observe)
{
	int ret;

	zassert_equal(test_histogram_m.sum, 0, "Histogram value is not 0");

	ret = prometheus_histogram_observe(&test_histogram_m, 1);
	zassert_ok(ret, "Error observing histogram");

	zassert_equal(test_histogram_m.sum, 1.0, "Histogram value is not 1");

	ret = prometheus_histogram_observe(&test_histogram_m, 2);
	zassert_ok(ret, "Error observing histogram");

	zassert_equal(test_histogram_m.sum, 3.0, "Histogram value is not 2");
}

ZTEST_SUITE(test_histogram, NULL, NULL, NULL, NULL, NULL);
