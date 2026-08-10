/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>
#include <string.h>

#include "ams_as7341.h"

/*
 * Fixture struct MUST be named <suite_name>_fixture.
 * Suite name is "as7341_tests", so struct must be "as7341_tests_fixture".
 */
struct as7341_tests_fixture {
	const struct device *dev;
};

static void *as7341_setup(void)
{
	struct as7341_tests_fixture *fixture = malloc(sizeof(struct as7341_tests_fixture));

	zassume_not_null(fixture, NULL);

	fixture->dev = DEVICE_DT_GET_ANY(ams_as7341);
	zassume_not_null(fixture->dev, "AS7341 device not found in DT");
	zassume_true(device_is_ready(fixture->dev), "AS7341 device is not ready");

	return fixture;
}

static void as7341_before(void *f)
{
	ARG_UNUSED(f);
}

static void as7341_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(as7341_tests, NULL, as7341_setup, as7341_before, NULL, as7341_teardown);

/**
 * @brief Test that the AS7341 device is ready after initialisation.
 */
ZTEST_F(as7341_tests, test_device_ready)
{
	zassert_true(device_is_ready(fixture->dev), "AS7341 device should be ready");
}

/**
 * @brief Test that sample_fetch succeeds for SENSOR_CHAN_ALL.
 */
ZTEST_F(as7341_tests, test_sample_fetch_all)
{
	int ret = sensor_sample_fetch(fixture->dev);

	zassert_equal(ret, 0, "sensor_sample_fetch failed: %d", ret);
}

/**
 * @brief Test that all spectral channels return valid (non-negative) values.
 */
ZTEST_F(as7341_tests, test_channel_get_spectral)
{
	struct sensor_value val;
	int ret;

	ret = sensor_sample_fetch(fixture->dev);
	zassert_equal(ret, 0, "sensor_sample_fetch failed: %d", ret);

	/* F1 - 415 nm */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_415NM_F1,
				 &val);
	zassert_equal(ret, 0, "channel_get F1 failed: %d", ret);
	zassert_true(val.val1 >= 0, "F1 value should be non-negative");

	/* F2 - 445 nm */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_445NM_F2,
				 &val);
	zassert_equal(ret, 0, "channel_get F2 failed: %d", ret);
	zassert_true(val.val1 >= 0, "F2 value should be non-negative");

	/* F3 - 480 nm */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_480NM_F3,
				 &val);
	zassert_equal(ret, 0, "channel_get F3 failed: %d", ret);
	zassert_true(val.val1 >= 0, "F3 value should be non-negative");

	/* F4 - 515 nm */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_515NM_F4,
				 &val);
	zassert_equal(ret, 0, "channel_get F4 failed: %d", ret);
	zassert_true(val.val1 >= 0, "F4 value should be non-negative");

	/* Clear (pass 1) */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_CLEAR_0,
				 &val);
	zassert_equal(ret, 0, "channel_get CLEAR_0 failed: %d", ret);
	zassert_true(val.val1 >= 0, "CLEAR_0 value should be non-negative");

	/* NIR (pass 1) */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_NIR_0, &val);
	zassert_equal(ret, 0, "channel_get NIR_0 failed: %d", ret);
	zassert_true(val.val1 >= 0, "NIR_0 value should be non-negative");

	/* F5 - 555 nm */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_555NM_F5,
				 &val);
	zassert_equal(ret, 0, "channel_get F5 failed: %d", ret);
	zassert_true(val.val1 >= 0, "F5 value should be non-negative");

	/* F6 - 590 nm */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_590NM_F6,
				 &val);
	zassert_equal(ret, 0, "channel_get F6 failed: %d", ret);
	zassert_true(val.val1 >= 0, "F6 value should be non-negative");

	/* F7 - 630 nm */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_630NM_F7,
				 &val);
	zassert_equal(ret, 0, "channel_get F7 failed: %d", ret);
	zassert_true(val.val1 >= 0, "F7 value should be non-negative");

	/* F8 - 680 nm */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_680NM_F8,
				 &val);
	zassert_equal(ret, 0, "channel_get F8 failed: %d", ret);
	zassert_true(val.val1 >= 0, "F8 value should be non-negative");

	/* Clear (pass 2) */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_CLEAR, &val);
	zassert_equal(ret, 0, "channel_get CLEAR failed: %d", ret);
	zassert_true(val.val1 >= 0, "CLEAR value should be non-negative");

	/* NIR (pass 2) */
	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_NIR, &val);
	zassert_equal(ret, 0, "channel_get NIR failed: %d", ret);
	zassert_true(val.val1 >= 0, "NIR value should be non-negative");
}

/**
 * @brief Test attr_set and attr_get round-trip for GAIN.
 */
ZTEST_F(as7341_tests, test_attr_gain)
{
	struct sensor_value set_val = {.val1 = AS7341_GAIN_64X, .val2 = 0};
	struct sensor_value get_val;
	int ret;

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_GAIN, &set_val);
	zassert_equal(ret, 0, "attr_set GAIN failed: %d", ret);

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_GAIN, &get_val);
	zassert_equal(ret, 0, "attr_get GAIN failed: %d", ret);
	zassert_equal(get_val.val1, AS7341_GAIN_64X, "GAIN round-trip mismatch");
}

/**
 * @brief Test attr_set and attr_get round-trip for ATIME.
 */
ZTEST_F(as7341_tests, test_attr_atime)
{
	struct sensor_value set_val = {.val1 = 50, .val2 = 0};
	struct sensor_value get_val;
	int ret;

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY,
			      &set_val);
	zassert_equal(ret, 0, "attr_set ATIME failed: %d", ret);

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY,
			      &get_val);
	zassert_equal(ret, 0, "attr_get ATIME failed: %d", ret);
	zassert_equal(get_val.val1, 50, "ATIME round-trip mismatch");
}

/**
 * @brief Test attr_set and attr_get round-trip for ASTEP.
 */
ZTEST_F(as7341_tests, test_attr_astep)
{
	struct sensor_value set_val = {.val1 = 499, .val2 = 0};
	struct sensor_value get_val;
	int ret;

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_RESOLUTION, &set_val);
	zassert_equal(ret, 0, "attr_set ASTEP failed: %d", ret);

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_RESOLUTION, &get_val);
	zassert_equal(ret, 0, "attr_get ASTEP failed: %d", ret);
	zassert_equal(get_val.val1, 499, "ASTEP round-trip mismatch");
}

/**
 * @brief Test that an unsupported attribute returns -ENOTSUP.
 */
ZTEST_F(as7341_tests, test_attr_unsupported)
{
	struct sensor_value val = {.val1 = 0, .val2 = 0};
	int ret;

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_ALERT, &val);
	zassert_equal(ret, -ENOTSUP, "Expected -ENOTSUP for unsupported attr, got %d", ret);
}

/**
 * @brief Test that fetching an unsupported channel returns -ENOTSUP.
 */
ZTEST_F(as7341_tests, test_sample_fetch_unsupported_chan)
{
	int ret = sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP);

	zassert_equal(ret, -ENOTSUP, "Expected -ENOTSUP for unsupported channel, got %d", ret);
}

/**
 * @brief Test flicker detection fetch and channel_get.
 */
ZTEST_F(as7341_tests, test_flicker_detection)
{
	struct sensor_value val;
	int ret;

	ret = sensor_sample_fetch_chan(fixture->dev,
				       (enum sensor_channel)SENSOR_CHAN_AS7341_FLICKER);
	zassert_equal(ret, 0, "flicker fetch failed: %d", ret);

	ret = sensor_channel_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_AS7341_FLICKER,
				 &val);
	zassert_equal(ret, 0, "flicker channel_get failed: %d", ret);

	/* Valid results: 0 (none), 1 (unknown), 100 Hz, 120 Hz */
	zassert_true(val.val1 == 0 || val.val1 == 1 || val.val1 == 100 || val.val1 == 120,
		     "Unexpected flicker value: %d", val.val1);
}
