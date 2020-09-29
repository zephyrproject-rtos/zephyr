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

#include <ztest.h>
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
 * @details
 * - get multiple channels values consistently in two operations:
 * fetch sample and get the values of each channel individually.
 * - check the results with sensor_value type avoids use of
 * floating point values
 *
 * @ingroup driver_sensor_subsys_tests
 *
 * @see sensor_sample_fetch(). sensor_channel_get()
 */
void test_sensor_get_channels(void)
{
	const struct device *dev;
	struct sensor_value data;

	dev = device_get_binding(DUMMY_SENSOR_NAME);
	zassert_not_null(dev, "failed: dev is null.");

	zassert_equal(sensor_sample_fetch(dev), RETURN_SUCCESS,
			"fail to fetch sample.");

	/* Get and check channels value. */
	for (int i = 0; i < TOTAL_CHAN_ELEMENTS; i++) {
		zassert_equal(sensor_channel_get(dev, chan_elements[i].chan,
				&data), RETURN_SUCCESS, "fail to get channel.");

		zassert_equal(data.val1, chan_elements[i].data.val1,
				"the data is not match.");
		zassert_equal(data.val2, chan_elements[i].data.val2,
				"the data is not match.");
	}
}

static void trigger_handler(const struct device *dev,
				struct sensor_trigger *trigger)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trigger);

	k_sem_give(&sem);
}

/**
 * @brief Test sensor multiple triggers
 *
 * @details
 * - set multiple triggers for dummy sensor
 * - Handle different types of triggers, based on time, data,threshold,
 * based on a delta value, near/far events and single/double tap.
 * - Configuration of runtime parameters in a sensor,
 * for example threshold values for interrupts.
 *
 * @ingroup driver_sensor_subsys_tests
 *
 * @see sensor_attr_set(). sensor_trigger_set()
 */
void test_sensor_handle_triggers(void)
{
	const struct device *dev;
	struct sensor_value data;

	dev = device_get_binding(DUMMY_SENSOR_NAME);
	zassert_not_null(dev, "failed: dev is null.");

	zassert_equal(sensor_sample_fetch(dev), RETURN_SUCCESS,
			"fail to fetch sample.");

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

		/* setting a sensor’s trigger and handler */
		zassert_equal(sensor_trigger_set(dev,
				&trigger_elements[i].trig,
				trigger_handler),
				RETURN_SUCCESS, "fail to set trigger");

		/* get channels value after trigger fired */
		k_sem_take(&sem, K_FOREVER);
		zassert_equal(sensor_channel_get(dev,
				trigger_elements[i].trig.chan,
				&data), RETURN_SUCCESS, "fail to get channel.");

		/* check the result of the trigger channel */
		zassert_equal(data.val1, trigger_elements[i].data.val1,
				"retrived data is not match.");
		zassert_equal(data.val2, trigger_elements[i].data.val2,
				"retrived data is not match.");
	}
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_sensor_api,
			 ztest_1cpu_unit_test(test_sensor_get_channels),
			 ztest_1cpu_unit_test(test_sensor_handle_triggers));

	ztest_run_test_suite(test_sensor_api);
}
