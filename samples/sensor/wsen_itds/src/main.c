/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <sys/printk.h>
#include <sys_clock.h>
#include <stdio.h>

#include <device.h>
#include <drivers/sensor.h>
#include <drivers/i2c.h>

#define MAX_TEST_TIME	5000
#define SLEEPTIME	300

static void print_accl_data(const struct device *itds)
{
	struct sensor_value val[3];

	if (sensor_channel_get(itds, SENSOR_CHAN_ACCEL_XYZ, val) < 0) {
		printf("Cannot read itds accl channels.\n");
		return;
	}

	printf("Accl (m/s): X=%f, Y=%f, Z=%f\n",
		sensor_value_to_double(&val[0]),
		sensor_value_to_double(&val[1]),
		sensor_value_to_double(&val[2]));
}

static void print_temp_data(const struct device *itds)
{
	struct sensor_value val;

	if (sensor_channel_get(itds, SENSOR_CHAN_DIE_TEMP, &val) < 0) {
		printf("Temperature channel read error.\n");
		return;
	}

	printf("Temperature (Celsius): %f\n",
		sensor_value_to_double(&val));
}

static void test_polling_mode(const struct device *itds)
{
	int32_t remaining_test_time = MAX_TEST_TIME;

	do {
		if (sensor_sample_fetch(itds) < 0) {
			printf("accl sample update error.\n");
		} else {
			print_accl_data(itds);

			print_temp_data(itds);
		}

		/* wait a while */
		k_sleep(K_MSEC(SLEEPTIME));

		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);
}

#if defined(CONFIG_ITDS_TRIGGER)
static void trigger_handler(const struct device *itds,
			    struct sensor_trigger *trigger)
{
	switch (trigger->type) {
	case SENSOR_TRIG_DATA_READY:
		printf("Data ready.\n");
		if (sensor_sample_fetch(itds) < 0) {
			printf("sample update error.\n");
		} else {
			print_accl_data(itds);
		}
		return;

	default:
		printf("trigger handler: unknown trigger type.\n");
		return;
	}
}
#endif

static void test_trigger_mode(const struct device *itds)
{
#if defined(CONFIG_ITDS_TRIGGER)
	struct sensor_trigger trig;
	struct sensor_value attr;

	printf("Testing data ready trigger.\n");

	attr.val1 = 400;
	attr.val2 = 0;

	if (sensor_attr_set(itds, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &attr) < 0) {
		printf("cannot set sampling frequency.\n");
		return;
	}

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;

	if (sensor_trigger_set(itds, &trig, trigger_handler) < 0) {
		printf("cannot set trigger.\n");
		return;
	}

	k_sleep(K_MSEC(MAX_TEST_TIME));

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	if (sensor_trigger_set(itds, &trig, NULL) < 0) {
		printf("cannot clear trigger.\n");
		return;
	}

	printf("Data ready trigger test finished.\n");
#endif

}

void main(void)
{
	const struct device *itds;
	struct sensor_value attr;

	printf("get device wsen-itds\n");
	itds = device_get_binding(DT_LABEL(DT_INST(0, we_wsen_itds)));
	if (!itds) {
		printf("Device not found.\n");
		return;
	}

	/*
	 * Set accl range to +/- 16 G. Since the sensor API needs SI
	 * units, convert the range to rad/s.
	 */
	sensor_g_to_ms2(4, &attr);

	if (sensor_attr_set(itds, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &attr) < 0) {
		printf("Cannot set accl range.\n");
		return;
	}

	printf("Testing the polling mode.\n");
	test_polling_mode(itds);
	printf("Polling mode test finished.\n");

	printf("Testing the trigger mode.\n");
	test_trigger_mode(itds);
	printf("Trigger mode test finished.\n");
}
