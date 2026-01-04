/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include "adxl345.h"

extern struct sensor_driver_api *unit_test_get_device_api(void);
extern int unit_test_invoke_adxl345_init(const struct device *dev);

static struct device dev;
static struct adxl345_dev_data dev_data;

static void *suite_setup(void)
{
	dev.api = unit_test_get_device_api();
	dev.data = &dev_data;

	/* Set the values that would be read via SPI or I2C from the device. */
	dev_data.samples.x = 0;
	dev_data.samples.y = 100;
	dev_data.samples.z = -100;

	return NULL; /* No fixture. */
}

ZTEST(adxl345_accel_convert, test_convert_10bit_right_justified_2g_mode)
{
	struct sensor_value data[3] = {0};
	/* Set the range which is normally configured via DeviceTree. */
	dev_data.selected_range = ADXL345_RANGE_2G;

	/* Do the conversion. */
	z_impl_sensor_channel_get((const struct device *)&dev, SENSOR_CHAN_ACCEL_XYZ, data);

	/* Check x-axis. */
	zassert_equal(data[0].val1, 0, "expected 0, was %d", data[0].val1);
	zassert_equal(data[0].val2, 0, "expected 0, was %d", data[0].val2);

	/* Check y-axis. */
	zassert_equal(data[1].val1, 3, "expected 3, was %d", data[1].val1);
	zassert_equal(data[1].val2, 830700, "expected 830700, was %d", data[1].val2);

	/* Check z-axis. */
	zassert_equal(data[2].val1, -3, "expected -3, was %d", data[2].val1);
	zassert_equal(data[2].val2, -830700, "expected -830700, was %d", data[2].val2);
}

ZTEST(adxl345_accel_convert, test_convert_10bit_right_justified_4g_mode)
{
	struct sensor_value data[3] = {0};
	/* Set the range which is normally configured via DeviceTree. */
	dev_data.selected_range = ADXL345_RANGE_4G;

	/* Do the conversion. */
	z_impl_sensor_channel_get((const struct device *)&dev, SENSOR_CHAN_ACCEL_XYZ, data);

	/* Check x-axis. */
	zassert_equal(data[0].val1, 0, "expected 0, was %d", data[0].val1);
	zassert_equal(data[0].val2, 0, "expected 0, was %d", data[0].val2);

	/* Check y-axis. */
	zassert_equal(data[1].val1, 7, "expected 7, was %d", data[1].val1);
	zassert_equal(data[1].val2, 661400, "expected 661400, was %d", data[1].val2);

	/* Check z-axis. */
	zassert_equal(data[2].val1, -7, "expected -7, was %d", data[2].val1);
	zassert_equal(data[2].val2, -661400, "expected -661400, was %d", data[2].val2);
}

ZTEST_SUITE(adxl345_accel_convert, NULL, suite_setup, NULL, NULL, NULL);
