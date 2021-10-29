/*
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>

#define MAX_TEST_TIME 1500
#define SLEEPTIME 300
#define KX022_TILT_POS_FU 0x01
#define KX022_TILT_POS_FD 0x02
#define KX022_TILT_POS_UP 0x04
#define KX022_TILT_POS_DO 0x08
#define KX022_TILT_POS_RI 0x010
#define KX022_TILT_POS_LE 0x20
#define KX022_ZPWU 0x01
#define KX022_ZNWU 0x02
#define KX022_YPMU 0x04
#define KX022_YNWU 0x08
#define KX022_XPWU 0x010
#define KX022_XNWU 0x20
static uint8_t slope_test_done = 0;
static uint8_t motion_test_done = 0;

static void tilt_position(struct sensor_value *value)
{
	uint32_t data;

	data = (uint8_t)value->val1;

	if (data == KX022_TILT_POS_LE) {
		printf(" Tilt Position: Face-Up(Z+)\r\n");
	} else if (data == KX022_TILT_POS_RI) {
		printf(" Tilt Position: Face-Down(Z-)\r\n");
	} else if (data == KX022_TILT_POS_DO) {
		printf(" Tilt Position: Up(Y+)\r\n");
	} else if (data == KX022_TILT_POS_UP) {
		printf(" Tilt Position: Down(Y-)\r\n");
	} else if (data == KX022_TILT_POS_FD) {
		printf(" Tilt Position: Right(X+)\r\n");
	} else if (data == KX022_TILT_POS_FU) {
		printf(" Tilt Position: Left (X-)\r\n");
	} else {
		printf("Not support for multiple axis\r\n");
	}
}

static void motion_direction(struct sensor_value *value)
{
	uint32_t data;

	data = (uint8_t)value->val1;

	if (data == KX022_ZPWU) {
		printf("Z+\r\n");
	} else if (data == KX022_ZNWU) {
		printf("Z-\r\n");
	} else if (data == KX022_YPMU) {
		printf("Y+\r\n");
	} else if (data == KX022_YNWU) {
		printf("Y-\r\n");
	} else if (data == KX022_XPWU) {
		printf("X+\r\n");
	} else if (data == KX022_XNWU) {
		printf("X-\r\n");
	} else {
		printf("Not support for multiple axis\r\n");
	}
}
static void fetch_and_display(const struct device *sensor)
{
	static unsigned int count;
	struct sensor_value accel[3];
	const char *overrun = "";
	int rc = sensor_sample_fetch(sensor);

	++count;
	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_KX022_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_XYZ, accel);
	}

	if (rc < 0) {
		printf("ERROR: Update failed: %d\n", rc);
	} else {
		printf("#%u @ %u ms: %sx %f , y %f , z %f\n", count, k_uptime_get_32(), overrun,
		       sensor_value_to_double(&accel[0]), sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]));
	}
}
static void motion_display(const struct device *sensor)
{
	struct sensor_value rd_data[1];
	const char *overrun = "";
	int rc = sensor_sample_fetch(sensor);

	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_KX022_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor, SENSOR_CHAN_FREE_FALL, rd_data);
	}
	if (rc < 0) {
		printf("ERROR: Update failed: %d\n", rc);
	} else {
		printf("Motion Direction :\t");
		motion_direction(&rd_data[0]);
	}
}
static void tilt_position_display(const struct device *sensor)
{
	struct sensor_value rd_data[2];
	const char *overrun = "";
	int rc = sensor_sample_fetch(sensor);

	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_KX022_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor, SENSOR_CHAN_NEAR_FAR, rd_data);
	}
	if (rc < 0) {
		printf("ERROR: Update failed: %d\n", rc);
	} else {
		printf("Previous Position :\t");
		tilt_position(&rd_data[0]);
		printf("Current Position :\t");
		tilt_position(&rd_data[1]);
	}
}

#ifdef CONFIG_KX022_TRIGGER
static void trigger_handler(const struct device *dev, struct sensor_trigger *trig)
{
	fetch_and_display(dev);
	motion_display(dev);
	tilt_position_display(dev);
}

static void motion_handler(const struct device *dev, struct sensor_trigger *trig)
{
	static unsigned int motion_cnt;

	fetch_and_display(dev);
	motion_display(dev);
	if (++motion_cnt > 5) {
		motion_test_done = 1;
		motion_cnt = 0;
	}
}
static void slope_handler(const struct device *dev, struct sensor_trigger *trig)
{
	static unsigned int slope_cont ;

	fetch_and_display(dev);
	tilt_position_display(dev);
	if (++slope_cont > 5) {
		slope_test_done = 1;
		slope_cont = 0;
	}
}
#endif

static void test_polling_mode(const struct device *dev)
{
	int32_t remaining_test_time = MAX_TEST_TIME;

	printf("\n");
	printf("\t\tAccelerometer: Poling test Start\r\n");
	do {
		fetch_and_display(dev);
		/* wait a while */
		k_msleep(SLEEPTIME);

		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);
}
static void test_trigger_mode(const struct device *dev)
{
	struct sensor_trigger trig;
	uint8_t rc;

	trig.type = SENSOR_TRIG_NEAR_FAR;
	trig.chan = SENSOR_CHAN_ALL;

	rc = sensor_trigger_set(dev, &trig, slope_handler);
	printf("\n");
	printf("\t\tAccelerometer: Tilt Position trigger test Start\r\n");

	while (slope_test_done == 0) {
		k_sleep(K_MSEC(200));
	}
	slope_test_done = 0;
	printf("\n");
	printf("\t\tAccelerometer: Tilt Position trigger test finished\r\n");

	trig.type = SENSOR_TRIG_FREEFALL;
	trig.chan = SENSOR_CHAN_ALL;

	rc = sensor_trigger_set(dev, &trig, motion_handler);
	printf("\n");
	printf("\t\tAccelerometer: Motion  trigger test Start\r\n");
	while (motion_test_done == 0) {
		k_sleep(K_MSEC(200));
	}
	motion_test_done = 0;
	printf("\n");
	printf("\t\tAccelerometer: Motion trigger test finished\r\n");
	rc = sensor_attr_set(dev, SENSOR_CHAN_FREE_FALL, SENSOR_ATTR_WUFF_TH, 0);
	k_sleep(K_MSEC(500));
}

void main(void)
{
	const struct device *sensor = device_get_binding(DT_LABEL(DT_INST(0, kionix_kx022)));

	if (sensor == NULL) {
		printf("Could not get %s device\n", DT_LABEL(DT_INST(0, kionix_kx022)));
		return;
	}

	while (true) {
		test_polling_mode(sensor);
		k_msleep(50);
		test_trigger_mode(sensor);
	}
}
