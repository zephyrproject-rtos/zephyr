/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

static const struct device *temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp_sensor));
static enum sensor_channel chan_to_use; /* this is filled by before() */

static volatile bool trigger_handler_called;

ZTEST(temp_sensor, test_polling)
{
	int rc;
	int cnt;
	struct sensor_value val;

	cnt = 0;
	while (1) {
		int32_t temp_val;

		rc = sensor_sample_fetch_chan(temp_dev, chan_to_use);
		zassert_ok(rc, "Cannot fetch chan sample: %d.", rc);

		rc = sensor_channel_get(temp_dev, chan_to_use, &val);
		zassert_ok(rc, "Cannot read from channel %d: %d.",
			   chan_to_use, rc);

		temp_val = (val.val1 * 100) + (val.val2 / 10000);
		TC_PRINT("Temperature: %d.%02u\n",
			temp_val/100, abs(temp_val) % 100);

		++cnt;
		if (cnt >= 5) {
			break;
		}

		k_sleep(K_MSEC(500));
	}
}

static void trigger_handler(const struct device *temp_dev,
			    const struct sensor_trigger *trig)
{
	ARG_UNUSED(temp_dev);
	ARG_UNUSED(trig);

	trigger_handler_called = true;
}

ZTEST(temp_sensor, test_trigger)
{
	int rc;
	struct sensor_value val;
	struct sensor_trigger trig = { .type = SENSOR_TRIG_THRESHOLD,
				       .chan = chan_to_use };

	/* Check if the sensor allows setting a threshold trigger.
	 * If not, skip the test.
	 */
	rc = sensor_trigger_set(temp_dev, &trig, NULL);
	if (rc == -ENOSYS || rc == -ENOTSUP) {
		TC_PRINT("This sensor does not support threshold trigger.\n");
		ztest_test_skip();
	}

	rc = sensor_channel_get(temp_dev, chan_to_use, &val);
	zassert_ok(rc, "Cannot read from channel %d: %d.",
			chan_to_use, rc);

	/* Set the upper threshold somewhat below the temperature read above. */
	val.val1 -= 5;
	rc = sensor_attr_set(temp_dev, chan_to_use,
			     SENSOR_ATTR_UPPER_THRESH, &val);
	zassert_ok(rc, "Cannot set upper threshold: %d.", rc);

	/* And the lower threshold below the upper one. */
	val.val1 -= 1;
	rc = sensor_attr_set(temp_dev, chan_to_use,
			     SENSOR_ATTR_LOWER_THRESH, &val);
	zassert_ok(rc, "Cannot set lower threshold: %d.", rc);

	/* Set sampling frequency to 10 Hz, to expect a trigger after 100 ms. */
	val.val1 = 10;
	val.val2 = 0;
	rc = sensor_attr_set(temp_dev, chan_to_use,
			     SENSOR_ATTR_SAMPLING_FREQUENCY, &val);
	zassert_ok(rc, "Cannot set sampling frequency: %d.", rc);

	trigger_handler_called = false;

	rc = sensor_trigger_set(temp_dev, &trig, trigger_handler);
	zassert_ok(rc, "Cannot enable the trigger: %d.", rc);

	k_sleep(K_MSEC(300));
	zassert_true(trigger_handler_called);

	rc = sensor_trigger_set(temp_dev, &trig, NULL);
	zassert_ok(rc, "Cannot disable the trigger: %d.", rc);

	trigger_handler_called = false;

	k_sleep(K_MSEC(300));
	zassert_false(trigger_handler_called);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	int rc;
	int cnt;
	struct sensor_value val;

	zassert_true(device_is_ready(temp_dev),
		"Device %s is not ready.", temp_dev->name);

	cnt = 0;
	/* Try to fetch a sample to check if the sensor is ready to work.
	 * Try several times if it appears to be needing a while for some
	 * initialization of communication etc.
	 */
	while (1) {
		rc = sensor_sample_fetch(temp_dev);
		if (rc != -EAGAIN && rc != -ENOTCONN) {
			break;
		}

		++cnt;
		zassert_false(cnt >= 3, "Cannot fetch a sample: %d.", rc);

		k_sleep(K_MSEC(1000));
	}
	zassert_ok(rc, "Cannot fetch a sample: %d.", rc);

	/* Check if the sensor provides the die temperature.
	 * If not, switch to the ambient one.
	 */
	chan_to_use = SENSOR_CHAN_DIE_TEMP;
	rc = sensor_channel_get(temp_dev, chan_to_use, &val);
	if (rc == -ENOTSUP) {
		chan_to_use = SENSOR_CHAN_AMBIENT_TEMP;
	}
}

ZTEST_SUITE(temp_sensor, NULL, NULL, before, NULL, NULL);
