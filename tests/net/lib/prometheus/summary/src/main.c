/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/net/prometheus/summary.h>

PROMETHEUS_SUMMARY_DEFINE(test_summary_m, "Test summary",
			  ({ .key = "test", .value = "summary" }), NULL);

/**
 * @brief Test prometheus_summary_observe
 *
 * @details The test shall observe the histogram value by 1 and check if the
 * value is incremented correctly.
 *
 * @details The test shall observe the histogram value by 2 and check if the
 * value is incremented correctly.
 */
ZTEST(test_summary, test_summary_observe)
{
	int ret;

	zassert_equal(test_summary_m.sum, 0, "Histogram value is not 0");

	ret = prometheus_summary_observe(&test_summary_m, 1);
	zassert_ok(ret, "Error observing histogram");

	zassert_equal(test_summary_m.sum, 1, "Histogram value is not 1");

	ret = prometheus_summary_observe(&test_summary_m, 2);
	zassert_ok(ret, "Error observing histogram");

	zassert_equal(test_summary_m.sum, 3, "Histogram value is not 3");
}

ZTEST_SUITE(test_summary, NULL, NULL, NULL, NULL, NULL);
