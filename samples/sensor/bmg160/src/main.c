/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys_clock.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>

#define MAX_TEST_TIME	15000
#define SLEEPTIME	300

#if !DT_HAS_COMPAT_STATUS_OKAY(bosch_bmg160)
#error "No bosch,bmg160 compatible node found in the device tree"
#endif

static void print_gyro_data(const struct device *bmg160)
{
	struct sensor_value val[3];

	if (sensor_channel_get(bmg160, SENSOR_CHAN_GYRO_XYZ, val) < 0) {
		printf("Cannot read bmg160 gyro channels.\n");
		return;
	}

	printf("Gyro (rad/s): X=%f, Y=%f, Z=%f\n",
	       val[0].val1 + val[0].val2 / 1000000.0,
	       val[1].val1 + val[1].val2 / 1000000.0,
	       val[2].val1 + val[2].val2 / 1000000.0);
}

static void print_temp_data(const struct device *bmg160)
{
	struct sensor_value val;

	if (sensor_channel_get(bmg160, SENSOR_CHAN_DIE_TEMP, &val) < 0) {
		printf("Temperature channel read error.\n");
		return;
	}

	printf("Temperature (Celsius): %f\n",
	       val.val1 + val.val2 / 1000000.0);
}

static void test_polling_mode(const struct device *bmg160)
{
	int32_t remaining_test_time = MAX_TEST_TIME;

	do {
		if (sensor_sample_fetch(bmg160) < 0) {
			printf("Gyro sample update error.\n");
		}

		print_gyro_data(bmg160);

		print_temp_data(bmg160);

		/* wait a while */
		k_msleep(SLEEPTIME);

		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);
}

static void trigger_handler(const struct device *bmg160,
			    const struct sensor_trigger *trigger)
{
	if (trigger->type != SENSOR_TRIG_DATA_READY &&
	    trigger->type != SENSOR_TRIG_DELTA) {
		printf("Gyro: trigger handler: unknown trigger type.\n");
		return;
	}

	if (sensor_sample_fetch(bmg160) < 0) {
		printf("Gyro sample update error.\n");
	}

	print_gyro_data(bmg160);
}

static void test_trigger_mode(const struct device *bmg160)
{
	int32_t remaining_test_time = MAX_TEST_TIME;
	struct sensor_trigger trig;
	struct sensor_value attr;

	trig.type = SENSOR_TRIG_DELTA;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;

	printf("Gyro: Testing anymotion trigger.\n");

	/* set up the trigger */

	/* set slope threshold to 10 dps */

	sensor_degrees_to_rad(10, &attr); /* convert to rad/s */

	if (sensor_attr_set(bmg160, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SLOPE_TH, &attr) < 0) {
		printf("Gyro: cannot set slope threshold.\n");
		return;
	}

	/* set slope duration to 4 samples */
	attr.val1 = 4;
	attr.val2 = 0;

	if (sensor_attr_set(bmg160, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SLOPE_DUR, &attr) < 0) {
		printf("Gyro: cannot set slope duration.\n");
		return;
	}

	if (sensor_trigger_set(bmg160, &trig, trigger_handler) < 0) {
		printf("Gyro: cannot set trigger.\n");
		return;
	}

	printf("Gyro: rotate the device and wait for events.\n");
	do {
		k_msleep(SLEEPTIME);
		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);

	if (sensor_trigger_set(bmg160, &trig, NULL) < 0) {
		printf("Gyro: cannot clear trigger.\n");
		return;
	}

	printf("Gyro: Anymotion trigger test finished.\n");

	printf("Gyro: Testing data ready trigger.\n");

	attr.val1 = 100;
	attr.val2 = 0;

	if (sensor_attr_set(bmg160, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &attr) < 0) {
		printf("Gyro: cannot set sampling frequency.\n");
		return;
	}

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;

	if (sensor_trigger_set(bmg160, &trig, trigger_handler) < 0) {
		printf("Gyro: cannot set trigger.\n");
		return;
	}

	remaining_test_time = MAX_TEST_TIME;

	do {
		k_msleep(SLEEPTIME);
		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);

	if (sensor_trigger_set(bmg160, &trig, NULL) < 0) {
		printf("Gyro: cannot clear trigger.\n");
		return;
	}

	printf("Gyro: Data ready trigger test finished.\n");
}

void main(void)
{
	const struct device *bmg160 = DEVICE_DT_GET_ANY(bosch_bmg160);
#if defined(CONFIG_BMG160_RANGE_RUNTIME)
	struct sensor_value attr;
#endif

	if (!device_is_ready(bmg160)) {
		printf("Device %s is not ready.\n", bmg160->name);
		return;
	}

#if defined(CONFIG_BMG160_RANGE_RUNTIME)
	/*
	 * Set gyro range to +/- 250 degrees/s. Since the sensor API needs SI
	 * units, convert the range to rad/s.
	 */
	sensor_degrees_to_rad(250, &attr);

	if (sensor_attr_set(bmg160, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &attr) < 0) {
		printf("Cannot set gyro range.\n");
		return;
	}
#endif

	printf("Testing the polling mode.\n");
	test_polling_mode(bmg160);
	printf("Polling mode test finished.\n");

	printf("Testing the trigger mode.\n");
	test_trigger_mode(bmg160);
	printf("Trigger mode test finished.\n");
}
