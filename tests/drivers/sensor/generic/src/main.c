/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup driver_sensor_subsys_tests sensor_subsys
 * @ingroup all_tests
 * @{
 * @}
 */

#include <zephyr/ztest.h>
#include "dummy_sensor.h"

K_SEM_DEFINE(sem, 0, 1);
#define RETURN_SUCCESS  (0)

struct channel_sequence {
	enum sensor_channel chan;
	struct sensor_value data;
};

struct trigger_sequence {
	struct sensor_trigger trig;
	struct sensor_value data;
	enum sensor_attribute attr;
};

static struct channel_sequence chan_elements[] = {
	{ SENSOR_CHAN_LIGHT, { 0, 0 } },
	{ SENSOR_CHAN_RED, { 1, 1 } },
	{ SENSOR_CHAN_GREEN, { 2, 4 } },
	{ SENSOR_CHAN_BLUE, { 3, 9 } },
	{ SENSOR_CHAN_PROX, { 4, 16 } }
};

static struct trigger_sequence trigger_elements[] = {
	/* trigger for SENSOR_TRIG_THRESHOLD */
	{ {SENSOR_TRIG_THRESHOLD, SENSOR_CHAN_PROX},
	{ 127, 0 }, SENSOR_ATTR_UPPER_THRESH },

	/* trigger for SENSOR_TRIG_TIMER */
	{ {SENSOR_TRIG_TIMER, SENSOR_CHAN_PROX},
	{ 130, 127 }, SENSOR_ATTR_UPPER_THRESH },

	/* trigger for SENSOR_TRIG_DATA_READY */
	{ {SENSOR_TRIG_DATA_READY, SENSOR_CHAN_PROX},
	{ 150, 130 }, SENSOR_ATTR_UPPER_THRESH },

	/* trigger for SENSOR_TRIG_DELTA */
	{ {SENSOR_TRIG_DELTA, SENSOR_CHAN_PROX},
	{ 180, 150 }, SENSOR_ATTR_UPPER_THRESH },

	/* trigger for SENSOR_TRIG_NEAR_FAR */
	{ {SENSOR_TRIG_NEAR_FAR, SENSOR_CHAN_PROX},
	{ 155, 180 }, SENSOR_ATTR_UPPER_THRESH }
};

#define TOTAL_CHAN_ELEMENTS (sizeof(chan_elements) / \
		sizeof(struct channel_sequence))
#define TOTAL_TRIG_ELEMENTS (sizeof(trigger_elements) / \
		sizeof(struct trigger_sequence))

/**
 * @brief Test get multiple channels values.
 *
 * @ingroup driver_sensor_subsys_tests
 *
 * @details
 * Test Objective:
 * - get multiple channels values consistently in two operations:
 * fetch sample and get the values of each channel individually.
 * - check the results with sensor_value type avoids use of
 * floating point values
 *
 * Testing techniques:
 * - function and block box testing,Interface testing,
 * Dynamic analysis and testing, Equivalence classes.
 *
 * Prerequisite Conditions:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Define a device and bind to dummy sensor.
 * -# Fetch the sample of dummy senor and check the result.
 * -# Get SENSOR_CHAN_LIGHT/SENSOR_CHAN_RED/SENSOR_CHAN_GREEN/
 * SENSOR_CHAN_BLUE/SENSOR_CHAN_BLUE channels from the sensor,
 * and check the result.
 *
 * Expected Test Result:
 * - Application can get multiple channels for dummy sensor.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see sensor_sample_fetch(), sensor_channel_get()
 */
ZTEST(sensor_api, test_sensor_get_channels)
{
	const struct device *dev;
	struct sensor_value data;

	dev = device_get_binding(DUMMY_SENSOR_NAME);
	zassert_not_null(dev, "failed: dev is null");

	/* test fetch single channel */
	zassert_equal(sensor_sample_fetch_chan(dev, chan_elements[0].chan),
				RETURN_SUCCESS,	"fail to fetch sample");
	/* Get and check channel 0 value. */
	zassert_equal(sensor_channel_get(dev, chan_elements[0].chan,
				&data), RETURN_SUCCESS, "fail to get channel");
	zassert_equal(data.val1, chan_elements[0].data.val1,
				"the data does not match");
	zassert_equal(data.val2, chan_elements[0].data.val2,
				"the data does not match");

	/* test fetch all channel */
	zassert_equal(sensor_sample_fetch(dev), RETURN_SUCCESS,
			"fail to fetch sample");
	/* Get and check channels value except for chanel 0. */
	for (int i = 1; i < TOTAL_CHAN_ELEMENTS; i++) {
		zassert_equal(sensor_channel_get(dev, chan_elements[i].chan,
				&data), RETURN_SUCCESS, "fail to get channel");
		zassert_equal(data.val1, chan_elements[i].data.val1,
				"the data does not match");
		zassert_equal(data.val2, chan_elements[i].data.val2,
				"the data does not match");
	}

	/* Get data with invalid channel. */
	zassert_not_equal(sensor_channel_get(dev, SENSOR_CHAN_DISTANCE,
				&data), RETURN_SUCCESS, "should fail for invalid channel");
}

static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trigger)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trigger);

	k_sem_give(&sem);
}

/**
 * @brief Test sensor multiple triggers.
 *
 * @ingroup driver_sensor_subsys_tests
 *
 * @details
 * Test Objective:
 * Check if sensor subsys can set multiple triggers and
 * can set/get sensor attribute.
 *
 * Testing techniques:
 * - function and block box testing,Interface testing,
 * Dynamic analysis and testing.
 *
 * Prerequisite Conditions:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Define a device and bind to dummy sensor and
 * check the result.
 * -# set multiple triggers for the dummy sensor and no trig sensor.
 * then check the result.
 * -# Handle different types of triggers, based on time, data,threshold,
 * based on a delta value, near/far events and single/double tap and
 * check the result.
 *
 * Expected Test Result:
 * - Application can get multiple channels for dummy sensor.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see sensor_attr_set(), sensor_trigger_set()
 */
ZTEST(sensor_api, test_sensor_handle_triggers)
{
	const struct device *dev;
	const struct device *dev_no_trig;
	struct sensor_value data;

	dev = device_get_binding(DUMMY_SENSOR_NAME);
	dev_no_trig = device_get_binding(DUMMY_SENSOR_NAME_NO_TRIG);
	zassert_not_null(dev, "failed: dev is null");

	zassert_equal(sensor_sample_fetch(dev), RETURN_SUCCESS,
			"fail to fetch sample");

	/* setup multiple triggers */
	for (int i = 0; i < TOTAL_TRIG_ELEMENTS; i++) {
		/* set attributes for trigger */
		zassert_equal(sensor_attr_set(dev,
				trigger_elements[i].trig.chan,
				trigger_elements[i].attr,
				&trigger_elements[i].data),
				RETURN_SUCCESS, "fail to set attributes");

		/* read-back attributes for trigger */
		zassert_equal(sensor_attr_get(dev,
				trigger_elements[i].trig.chan,
				trigger_elements[i].attr,
				&data),
				RETURN_SUCCESS, "fail to get attributes");
		zassert_equal(trigger_elements[i].data.val1,
			      data.val1, "read-back returned wrong val1");
		zassert_equal(trigger_elements[i].data.val2,
			      data.val2, "read-back returned wrong val2");

		/* setting a sensor's trigger and handler */
		zassert_equal(sensor_trigger_set(dev,
				&trigger_elements[i].trig,
				trigger_handler),
				RETURN_SUCCESS, "fail to set trigger");

		/* get channels value after trigger fired */
		k_sem_take(&sem, K_FOREVER);
		zassert_equal(sensor_channel_get(dev,
				trigger_elements[i].trig.chan,
				&data), RETURN_SUCCESS, "fail to get channel");

		/* check the result of the trigger channel */
		zassert_equal(data.val1, trigger_elements[i].data.val1,
				"retrieved data does not match");
		zassert_equal(data.val2, trigger_elements[i].data.val2,
				"retrieved data does not match");

		/* set attributes for no trig dev */
		zassert_equal(sensor_attr_set(dev_no_trig,
				trigger_elements[i].trig.chan,
				trigger_elements[i].attr,
				&trigger_elements[i].data),
				-ENOSYS, "fail to set attributes");

		/* read-back attributes for no trig dev*/
		zassert_equal(sensor_attr_get(dev_no_trig,
				trigger_elements[i].trig.chan,
				trigger_elements[i].attr,
				&data),
				-ENOSYS, "fail to get attributes");

		/* setting a sensor's trigger and handler for no trig dev */
		zassert_equal(sensor_trigger_set(dev_no_trig,
				&trigger_elements[i].trig,
				trigger_handler),
				-ENOSYS, "fail to set trigger");
	}
}

/**
 * @brief Test unit conversion of sensor module
 * @details Verify helper function to convert acceleration from
 * Gs to m/s^2 and from m/s^2 to Gs.  Verify helper function
 * to convert radians to degrees and degrees to radians.  Verify
 * helper function for converting struct sensor_value to double.
 * Verify helper functions for converting to milli and micro prefix
 * units.
 */
ZTEST(sensor_api, test_sensor_unit_conversion)
{
	struct sensor_value data;

	/* Test acceleration unit conversion */
	sensor_g_to_ms2(1, &data);
	zassert_equal(data.val1, SENSOR_G/1000000LL,
			"the data does not match");
	zassert_equal(data.val2, SENSOR_G%(data.val1 * 1000000LL),
			"the data does not match");
	zassert_equal(sensor_ms2_to_g(&data), 1,
			"the data does not match");
	/* set test data to negative value */
	data.val1 = -data.val1;
	data.val2 = -data.val2;
	zassert_equal(sensor_ms2_to_g(&data), -1,
			"the data does not match");

	/* Test the conversion between angle and radian */
	sensor_degrees_to_rad(180, &data);
	zassert_equal(data.val1, SENSOR_PI/1000000LL,
			"the data does not match");
	zassert_equal(data.val2, SENSOR_PI%(data.val1 * 1000000LL),
			"the data does not match");
	zassert_equal(sensor_rad_to_degrees(&data), 180,
			"the data does not match");
	/* set test data to negative value */
	data.val1 = -data.val1;
	data.val2 = -data.val2;
	zassert_equal(sensor_rad_to_degrees(&data), -180,
			"the data does not match");

	/* reset test data to positive value */
	data.val1 = -data.val1;
	data.val2 = -data.val2;
#if defined(CONFIG_FPU)
	/* Test struct sensor_value to double and float */
	zassert_equal((long long)(sensor_value_to_double(&data) * 1000000LL),
			SENSOR_PI, "the data does not match");
	zassert_equal((long long)(sensor_value_to_float(&data) * 1000000LL),
			SENSOR_PI, "the data does not match");

	/* Test struct sensor_value from double and float */
	sensor_value_from_double(&data, (double)(SENSOR_PI) / 1000000.0);
	zassert_equal(data.val1, SENSOR_PI/1000000LL,
			"the data does not match");
	zassert_equal(data.val2, SENSOR_PI%(data.val1 * 1000000LL),
			"the data does not match");

	sensor_value_from_float(&data, (float)(SENSOR_PI) / 1000000.0f);
	zassert_equal(data.val1, SENSOR_PI/1000000LL,
			"the data does not match");
	zassert_equal(data.val2, SENSOR_PI%(data.val1 * 1000000LL),
			"the data does not match");
#endif
	/* reset test data to positive value */
	data.val1 = 3;
	data.val2 = 300000;
	zassert_equal(sensor_value_to_milli(&data), 3300LL,
			"the result does not match");
	zassert_equal(sensor_value_to_micro(&data), 3300000LL,
			"the result does not match");
	/* reset test data to negative value */
	data.val1 = -data.val1;
	data.val2 = -data.val2;
	zassert_equal(sensor_value_to_milli(&data), -3300LL,
			"the result does not match");
	zassert_equal(sensor_value_to_micro(&data), -3300000LL,
			"the result does not match");
	/* Test when result is greater than 32-bit wide */
	data.val1 = 5432109;
	data.val2 = 876543;
	zassert_equal(sensor_value_to_milli(&data), 5432109876LL,
			"the result does not match");
	zassert_equal(sensor_value_to_micro(&data), 5432109876543LL,
			"the result does not match");
}

ZTEST_SUITE(sensor_api, NULL, NULL, ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
