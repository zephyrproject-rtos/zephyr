/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 * Author: Mahendra Sondagar <mahendra@dotcom.co.in>
 * Author: Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 */
/**
 * @file
 * @brief Integration tests for the TMF8801 ToF sensor driver.
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>

#include "tmf8801.h"

/**
 * @brief Test fixture for the tmf8801_tests suite.
 *
 * Suite name is "tmf8801_tests".
 */
struct tmf8801_tests_fixture {
	const struct device *dev;
};

/**
 * @brief Suite setup: locate and verify the TMF8801 device.
 *
 * @return Pointer to an allocated ::tmf8801_tests_fixture.
 */
static void *tmf8801_setup(void)
{
	struct tmf8801_tests_fixture *fixture = malloc(sizeof(struct tmf8801_tests_fixture));

	zassume_not_null(fixture, NULL);

	fixture->dev = DEVICE_DT_GET_ANY(ams_tmf8801);
	zassume_not_null(fixture->dev, "TMF8801 device not found in DT");
	zassume_true(device_is_ready(fixture->dev), "TMF8801 device is not ready");

	return fixture;
}

/**
 * @brief Per-test hook, run before each test case.
 *
 * @param f Unused; required by the ZTEST_SUITE signature.
 */
static void tmf8801_before(void *f)
{
	ARG_UNUSED(f);
}

/**
 * @brief Suite teardown: free the test fixture.
 *
 * @param f Fixture allocated by tmf8801_setup().
 */
static void tmf8801_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(tmf8801_tests, NULL, tmf8801_setup, tmf8801_before, NULL, tmf8801_teardown);

/**
 * @brief Test that the TMF8801 device is ready after initialization.
 */
ZTEST_F(tmf8801_tests, test_device_ready)
{
	zassert_true(device_is_ready(fixture->dev), "TMF8801 device should be ready");
}

/**
 * @brief Test that distance sample fetch succeeds.
 */
ZTEST_F(tmf8801_tests, test_sample_fetch)
{
	int ret;

	ret = sensor_sample_fetch(fixture->dev);
	zassert_equal(ret, 0, "sensor_sample_fetch failed: %d", ret);
}

/**
 * @brief Test that the distance channel returns a valid value.
 */
ZTEST_F(tmf8801_tests, test_channel_get_distance)
{
	struct sensor_value val;
	int ret;

	ret = sensor_sample_fetch(fixture->dev);
	zassert_equal(ret, 0, "sensor_sample_fetch failed: %d", ret);

	ret = sensor_channel_get(fixture->dev, SENSOR_CHAN_DISTANCE, &val);
	zassert_equal(ret, 0, "channel_get DISTANCE failed: %d", ret);

	zassert_true(val.val1 >= 0, "Distance should be non-negative");
	zassert_true(val.val2 >= 0, "Distance fraction should be non-negative");
}

/**
 * @brief Test that SENSOR_CHAN_ALL sample fetch succeeds.
 */
ZTEST_F(tmf8801_tests, test_sample_fetch_all)
{
	int ret;

	ret = sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_ALL);
	zassert_equal(ret, 0, "sensor_sample_fetch_chan failed: %d", ret);
}

/**
 * @brief Test that unsupported channels return -ENOTSUP.
 */
ZTEST_F(tmf8801_tests, test_sample_fetch_unsupported_chan)
{
	int ret;

	ret = sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP);

	zassert_equal(ret, -ENOTSUP, "Expected -ENOTSUP for unsupported channel, got %d", ret);
}

/**
 * @brief Test that unsupported channel_get returns -ENOTSUP.
 */
ZTEST_F(tmf8801_tests, test_channel_get_unsupported)
{
	struct sensor_value val;
	int ret;

	ret = sensor_channel_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, &val);

	zassert_equal(ret, -ENOTSUP, "Expected -ENOTSUP for unsupported channel, got %d", ret);
}

/**
 * @brief Test repeated distance measurements.
 */
ZTEST_F(tmf8801_tests, test_repeated_distance_fetch)
{
	struct sensor_value val;
	int ret;

	for (int i = 0; i < 3; i++) {
		do {
			ret = sensor_sample_fetch(fixture->dev);
			if (ret == -EBUSY) {
				k_msleep(10);
			}
		} while (ret == -EBUSY);

		zassert_equal(ret, 0, "sensor_sample_fetch failed on sample %d: %d", i, ret);

		ret = sensor_channel_get(fixture->dev, SENSOR_CHAN_DISTANCE, &val);
		zassert_equal(ret, 0, "channel_get failed on sample %d: %d", i, ret);

		zassert_true(val.val1 >= 0, "Distance should be non-negative");
		zassert_true(val.val2 >= 0, "Distance fraction should be non-negative");
	}
}
