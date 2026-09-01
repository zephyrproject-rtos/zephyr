/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/safety_test/safety_test.h>

#include "sample_common.h"

static const struct device *const temp_dev = DEVICE_DT_GET(DT_ALIAS(ambient_temp0));

/*
 * This is the assumption the whole chain rests on: the POST_KERNEL sweep runs
 * at CONFIG_SAFETY_TEST_INIT_PRIORITY (99), the sensor initialises at
 * CONFIG_SENSOR_INIT_PRIORITY (90). Break the build if either is changed,
 * rather than failing mysteriously on the bench.
 */
BUILD_ASSERT(CONFIG_SAFETY_TEST_INIT_PRIORITY > CONFIG_SENSOR_INIT_PRIORITY,
	     "the safety sweep must run after the sensor initialises");

static int32_t last_mc;

int32_t sample_sensor_last_mc(void)
{
	return last_mc;
}

static int read_temperature(int32_t *out)
{
	struct sensor_value val;
	int ret;

	/*
	 * Guard every path into the driver, not just the first test to run. A
	 * failed init leaves the sensor's bus handle NULL and the driver
	 * dereferences it unchecked, so an unguarded fetch faults.
	 */
	if (!device_is_ready(temp_dev)) {
		return -ENODEV;
	}

	ret = sensor_sample_fetch_chan(temp_dev, SENSOR_CHAN_AMBIENT_TEMP);
	if (ret < 0) {
		return ret;
	}

	ret = sensor_channel_get(temp_dev, SENSOR_CHAN_AMBIENT_TEMP, &val);
	if (ret < 0) {
		return ret;
	}

	/* val1 is whole degrees, val2 is millionths. */
	*out = (val.val1 * 1000) + (val.val2 / 1000);

	return 0;
}

static int sensor_comm_run(const struct safety_test_context *ctx)
{
	int ret;

	ARG_UNUSED(ctx);

	ret = read_temperature(&last_mc);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int sensor_plausible_run(const struct safety_test_context *ctx)
{
	int ret;

	ARG_UNUSED(ctx);

	ret = read_temperature(&last_mc);
	if (ret < 0) {
		return ret;
	}

	if (last_mc < CONFIG_SAMPLE_TEMP_MIN_MC || last_mc > CONFIG_SAMPLE_TEMP_MAX_MC) {
		return -ERANGE;
	}

	return 0;
}

/*
 * Priority 20 puts this after clock_xcheck within the POST_KERNEL sweep, so a
 * bus fault caused by a wrong clock is attributed to the clock.
 */
SAFETY_TEST_DEFINE(sensor_comm, SAFETY_TEST_CAT_COMM, SAFETY_TEST_LEVEL_POST_KERNEL, 20,
		   SAFETY_TEST_FLAG_BOOT_OK | SAFETY_TEST_FLAG_RUNTIME_OK |
			   SAFETY_TEST_FLAG_CRITICAL,
		   sensor_comm_run, "P3T1755 responds on the I3C bus");

SAFETY_TEST_DEFINE(sensor_plausible, SAFETY_TEST_CAT_OTHER, SAFETY_TEST_LEVEL_APPLICATION, 10,
		   SAFETY_TEST_FLAG_BOOT_OK | SAFETY_TEST_FLAG_RUNTIME_OK |
			   SAFETY_TEST_FLAG_CRITICAL,
		   sensor_plausible_run, "Temperature within its plausibility band");
