/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 * Author: Mahendra Sondagar <mahendra@dotcom.co.in>
 * Author: Darshan Maru <darshan.maru@dnkmail.in>
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/tcs34725.h>

static const struct device *const tcs_dev = DEVICE_DT_GET_ANY(ams_tcs34725);

static void *tcs34725_setup(void)
{
	if (tcs_dev == NULL) {
		TC_PRINT("No TCS34725 device found in devicetree, skipping all tests\n");
		return NULL;
	}

	zassert_true(device_is_ready(tcs_dev), "Device is not ready");
	return NULL;
}

/**
 * @brief Test sample fetch and reading standard RGBC channels
 */
ZTEST(tcs34725, test_fetch_and_get_rgbc)
{
	if (tcs_dev == NULL) {
		ztest_test_skip();
		return;
	}

	struct sensor_value red, green, blue, clear;
	int rc;

	rc = sensor_sample_fetch(tcs_dev);
	zassert_ok(rc, "sensor_sample_fetch failed: %d", rc);

	rc = sensor_channel_get(tcs_dev, SENSOR_CHAN_RED, &red);
	zassert_ok(rc, "sensor_channel_get(RED) failed: %d", rc);

	rc = sensor_channel_get(tcs_dev, SENSOR_CHAN_GREEN, &green);
	zassert_ok(rc, "sensor_channel_get(GREEN) failed: %d", rc);

	rc = sensor_channel_get(tcs_dev, SENSOR_CHAN_BLUE, &blue);
	zassert_ok(rc, "sensor_channel_get(BLUE) failed: %d", rc);

	rc = sensor_channel_get(tcs_dev, SENSOR_CHAN_LIGHT, &clear);
	zassert_ok(rc, "sensor_channel_get(LIGHT) failed: %d", rc);

	TC_PRINT("Raw counts - R: %d, G: %d, B: %d, Clear: %d\n", red.val1, green.val1, blue.val1,
		 clear.val1);
}

/**
 * @brief Test reading custom channels (Lux and Color Temperature)
 */
ZTEST(tcs34725, test_fetch_custom_channels)
{
	if (tcs_dev == NULL) {
		ztest_test_skip();
		return;
	}

	struct sensor_value lux, color_temp;
	int rc;

	rc = sensor_sample_fetch(tcs_dev);
	zassert_ok(rc, "sensor_sample_fetch failed: %d", rc);

	rc = sensor_channel_get(tcs_dev, (enum sensor_channel)SENSOR_CHAN_TCS34725_LUX, &lux);
	zassert_ok(rc, "sensor_channel_get(LUX) failed: %d", rc);

	rc = sensor_channel_get(tcs_dev, (enum sensor_channel)SENSOR_CHAN_TCS34725_COLOR_TEMP,
				&color_temp);
	zassert_ok(rc, "sensor_channel_get(COLOR_TEMP) failed: %d", rc);

	TC_PRINT("Lux: %d (valid: %d), Color Temp: %d K (valid: %d)\n", lux.val1, lux.val2,
		 color_temp.val1, color_temp.val2);
}

/**
 * @brief Test setting driver attributes (Gain and Sampling Frequency / ATIME)
 */
ZTEST(tcs34725, test_attr_set)
{
	if (tcs_dev == NULL) {
		ztest_test_skip();
		return;
	}

	struct sensor_value attr;
	int rc;

	/* Test setting valid gain (0x01 = 4x gain) */
	attr.val1 = 1;
	attr.val2 = 0;
	rc = sensor_attr_set(tcs_dev, SENSOR_CHAN_ALL, SENSOR_ATTR_GAIN, &attr);
	zassert_ok(rc, "sensor_attr_set(GAIN) failed: %d", rc);

	/* Test setting invalid gain */
	attr.val1 = 5;
	attr.val2 = 0;
	rc = sensor_attr_set(tcs_dev, SENSOR_CHAN_ALL, SENSOR_ATTR_GAIN, &attr);
	zassert_equal(rc, -EINVAL, "Expected -EINVAL for invalid gain, got %d", rc);

	/* Test setting valid ATIME integration cycles */
	attr.val1 = 0xF6; /* ~24ms integration time */
	attr.val2 = 0;
	rc = sensor_attr_set(tcs_dev, SENSOR_CHAN_LIGHT, SENSOR_ATTR_SAMPLING_FREQUENCY, &attr);
	zassert_ok(rc, "sensor_attr_set(SAMPLING_FREQUENCY) failed: %d", rc);

	/* Test setting invalid ATIME */
	attr.val1 = 300;
	attr.val2 = 0;
	rc = sensor_attr_set(tcs_dev, SENSOR_CHAN_LIGHT, SENSOR_ATTR_SAMPLING_FREQUENCY, &attr);
	zassert_equal(rc, -EINVAL, "Expected -EINVAL for invalid ATIME, got %d", rc);
}

/**
 * @brief Test error handling for unsupported channel get requests
 */
ZTEST(tcs34725, test_unsupported_channel)
{
	if (tcs_dev == NULL) {
		ztest_test_skip();
		return;
	}

	struct sensor_value val;
	int rc;

	rc = sensor_channel_get(tcs_dev, SENSOR_CHAN_PROX, &val);
	zassert_equal(rc, -ENOTSUP, "Expected -ENOTSUP for unsupported channel, got %d", rc);
}

ZTEST_SUITE(tcs34725, NULL, tcs34725_setup, NULL, NULL, NULL);
