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

#define MAX_TEST_TIME	10000
#define SLEEPTIME	300

static void print_gyro_data(const struct device *bmx160)
{
	struct sensor_value val[3];

	if (sensor_channel_get(bmx160, SENSOR_CHAN_GYRO_XYZ, val) < 0) {
		printf("Cannot read bmx160 gyro channels.\n");
		return;
	}

	printf("Gyro (Grad): X=%f, Y=%f, Z=%f\n",
	       val[0].val1 + val[0].val2 / 1000000.0,
	       val[1].val1 + val[1].val2 / 1000000.0,
	       val[2].val1 + val[2].val2 / 1000000.0);
}

static void print_magn_data(const struct device *bmx160)
{
	struct sensor_value val[3];

	if (sensor_channel_get(bmx160, SENSOR_CHAN_MAGN_XYZ, val) < 0) {
		printf("Cannot read bmx160 magnetometer channels.\n");
		return;
	}

	printf("Magn (Gauss): X=%f, Y=%f, Z=%f\n",
	       val[0].val1 + val[0].val2 / 1000000.0,
	       val[1].val1 + val[1].val2 / 1000000.0,
	       val[2].val1 + val[2].val2 / 1000000.0);
}

static void print_accl_data(const struct device *bmx160)
{
	struct sensor_value val[3];

	if (sensor_channel_get(bmx160, SENSOR_CHAN_ACCEL_XYZ, val) < 0) {
		printf("Cannot read bmx160 accl channels.\n");
		return;
	}

	printf("Accl (m/s): X=%f, Y=%f, Z=%f\n",
	       val[0].val1 + val[0].val2 / 1000000.0,
	       val[1].val1 + val[1].val2 / 1000000.0,
	       val[2].val1 + val[2].val2 / 1000000.0);
}

static void print_temp_data(const struct device *bmx160)
{
	struct sensor_value val;

	if (sensor_channel_get(bmx160, SENSOR_CHAN_DIE_TEMP, &val) < 0) {
		printf("Temperature channel read error.\n");
		return;
	}

	printf("Temperature (Celsius): %f\n",
	       val.val1 + val.val2 / 1000000.0);
}

static void test_polling_mode(const struct device *bmx160)
{
	int32_t remaining_test_time = MAX_TEST_TIME;

	do {
		if (sensor_sample_fetch(bmx160) < 0) {
			printf("accl sample update error.\n");
		}

		print_accl_data(bmx160);

		print_gyro_data(bmx160);

		print_magn_data(bmx160);

		print_temp_data(bmx160);

		/* wait a while */
		k_sleep(K_MSEC(SLEEPTIME));

		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);
}

#if defined(CONFIG_BMI160_TRIGGER)
static void trigger_handler(const struct device *bmx160,
			    struct sensor_trigger *trigger)
{
	switch (trigger->type) {
	case SENSOR_TRIG_DOUBLE_TAP:
		printf("double tap event.\n");
		return;

	case SENSOR_TRIG_TAP:
		printf("single tap event.\n");
		return;

	case SENSOR_TRIG_DELTA:
		printf("any motion detected.\n");
		return;

	case SENSOR_TRIG_DATA_READY:
		printf("any drdy.\n");
		if (sensor_sample_fetch(bmx160) < 0) {
			printf("sample update error.\n");
		}

		print_accl_data(bmx160);
		print_gyro_data(bmx160);
		print_magn_data(bmx160);
		return;

	default:
		printf("trigger handler: unknown trigger type.\n");
		return;
	}
}
#endif

static void test_trigger_mode(const struct device *bmx160)
{
#if defined(CONFIG_BMI160_TRIGGER)
	int32_t remaining_test_time = MAX_TEST_TIME;
	struct sensor_trigger trig;
	struct sensor_value attr;

	printf("Testing anymotion trigger.\n");

	attr.val1 = 10;
	attr.val2 = 800000;
	if (sensor_attr_set(bmx160, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SLOPE_TH, &attr) < 0) {
		printf("cannot set slope threshold.\n");
		return;
	}

	/* set slope duration to 4 samples */
	attr.val1 = 4;
	attr.val2 = 0;

	if (sensor_attr_set(bmx160, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SLOPE_DUR, &attr) < 0) {
		printf("cannot set slope duration.\n");
		return;
	}

	/* set up the trigger */
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	trig.type = SENSOR_TRIG_DELTA;

	if (sensor_trigger_set(bmx160, &trig, trigger_handler) < 0) {
		printf("cannot set trigger.\n");
		return;
	}

	printf("move the device and wait for events.\n");
	do {
		k_sleep(K_MSEC(SLEEPTIME));
		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);

	if (sensor_trigger_set(bmx160, &trig, NULL) < 0) {
		printf("cannot clear trigger.\n");
		return;
	}

	printf("Anymotion trigger test finished.\n");

	printf("Testing tap trigger.\n");

#if defined(CONFIG_BMI160_ACCEL_ODR_RUNTIME)
	attr.val1 = 1600;
	attr.val2 = 0;

	if (sensor_attr_set(bmx160, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &attr) < 0) {
		printf("cannot set sampling frequency.\n");
		return;
	}
#endif

	attr.val1 = 30;
	attr.val2 = 0;

	if (sensor_attr_set(bmx160, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_LOWER_THRESH, &attr) < 0) {
		printf("cannot set tap lower thresh.\n");
		return;
	}

	attr.val1 = 200;
	attr.val2 = 0;

	if (sensor_attr_set(bmx160, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_UPPER_THRESH, &attr) < 0) {
		printf("cannot set tap upper thresh.\n");
		return;
	}

	trig.type = SENSOR_TRIG_TAP;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	if (sensor_trigger_set(bmx160, &trig, trigger_handler) < 0) {
		printf("cannot set double tap trigger.\n");
		return;
	}

	trig.type = SENSOR_TRIG_DOUBLE_TAP;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	if (sensor_trigger_set(bmx160, &trig, trigger_handler) < 0) {
		printf("cannot set double tap trigger.\n");
		return;
	}

	printf("Try single/double tap the device and wait for events.\n");
	remaining_test_time = MAX_TEST_TIME;
	do {
		k_sleep(K_MSEC(SLEEPTIME));
		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);

	if (sensor_trigger_set(bmx160, &trig, NULL) < 0) {
		printf("cannot clear trigger.\n");
		return;
	}

	trig.type = SENSOR_TRIG_TAP;
	if (sensor_trigger_set(bmx160, &trig, NULL) < 0) {
		printf("cannot clear trigger.\n");
		return;
	}

	printf("End of testing TAP.\n");

	printf("Testing data ready trigger.\n");

#if defined(CONFIG_BMI160_ACCEL_ODR_RUNTIME)
	attr.val1 = 100;
	attr.val2 = 0;

	if (sensor_attr_set(bmx160, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &attr) < 0) {
		printf("cannot set sampling frequency.\n");
		return;
	}
#endif
	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;

	if (sensor_trigger_set(bmx160, &trig, trigger_handler) < 0) {
		printf("cannot set trigger.\n");
		return;
	}

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;

	if (sensor_trigger_set(bmx160, &trig, trigger_handler) < 0) {
		printf("cannot set trigger.\n");
		return;
	}

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_MAGN_XYZ;

	if (sensor_trigger_set(bmx160, &trig, trigger_handler) < 0) {
		printf("cannot set trigger.\n");
		return;
	}

	remaining_test_time = MAX_TEST_TIME;
	do {
		k_sleep(K_MSEC(SLEEPTIME));
		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	if (sensor_trigger_set(bmx160, &trig, NULL) < 0) {
		printf("cannot clear trigger.\n");
		return;
	}

	trig.chan = SENSOR_CHAN_GYRO_XYZ;
	if (sensor_trigger_set(bmx160, &trig, NULL) < 0) {
		printf("cannot clear trigger.\n");
		return;
	}

	trig.chan = SENSOR_CHAN_MAGN_XYZ;
	if (sensor_trigger_set(bmx160, &trig, NULL) < 0) {
		printf("cannot clear trigger.\n");
		return;
	}

	printf("Data ready trigger test finished.\n");
#endif

}

void main(void)
{
	const struct device *bmx160;
#if defined(CONFIG_BMI160_ACCEL_RANGE_RUNTIME)
	struct sensor_value attr;
#endif

	printf("get device bmx160\n");
	bmx160 = device_get_binding("BMX160");
	if (!bmx160) {
		printf("Device not found.\n");
		return;
	}

#if defined(CONFIG_BMI160_ACCEL_RANGE_RUNTIME)
	/*
	 * Set accl range to +/- 16 G. Since the sensor API needs SI
	 * units, convert the range to rad/s.
	 */
	sensor_g_to_ms2(16, &attr);

	if (sensor_attr_set(bmx160, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &attr) < 0) {
		printf("Cannot set accl range.\n");
		return;
	}
#endif

	printf("Testing the polling mode.\n");
	test_polling_mode(bmx160);
	printf("Polling mode test finished.\n");

	printf("Testing the trigger mode.\n");
	test_trigger_mode(bmx160);
	printf("Trigger mode test finished.\n");
}
