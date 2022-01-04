/*
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <sys/reboot.h>
#include <drivers/sensor/kx022.h>

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
#define KX022_MASK_CNTL1_PC1 BIT(7)
#define KX022_MASK_CNTL1_RES BIT(6)
#define KX022_MASK_CNTL1_GSEL (BIT(4) | BIT(3))
#define KX022_MASK_CNTL1_TDTE BIT(2)
#define KX022_MASK_CNTL1_WUFE BIT(1)
#define KX022_MASK_CNTL1_TPE BIT(0)
#define KX022_MASK_ODCNTL_OSA (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define KX022_MASK_INC5_IEN2 BIT(5)
#define KX022_MASK_INC5_IEA2 BIT(4)
#define KX022_MASK_INC4_TDTI1 BIT(2)
#define KX022_MASK_INC4_WUFI1 BIT(1)
#define KX022_MASK_INC4_TPI1 BIT(0)
#define KX022_MASK_INC2_XNWUE BIT(5)
#define KX022_MASK_INC2_XPWUE BIT(4)
#define KX022_MASK_INC2_YNWUE BIT(3)
#define KX022_MASK_INC2_YPWUE BIT(2)
#define KX022_MASK_INC2_ZNWUE BIT(1)
#define KX022_MASK_INC2_ZPWUE BIT(0)
#define KX022_MASK_CNTL3_OWUF (BIT(2) | BIT(1) | BIT(0))
#define KX022_MASK_CNTL2_LEM BIT(5)
#define KX022_MASK_CNTL2_RIM BIT(4)
#define KX022_MASK_CNTL2_DOM BIT(3)
#define KX022_MASK_CNTL2_UPM BIT(2)
#define KX022_MASK_CNTL2_FDM BIT(1)
#define KX022_MASK_CNTL2_FUM BIT(0)
#define KX022_MASK_CNTL3_OTP (BIT(7) | BIT(6))
#define BITWISE_SHIFT_7 7
#define BITWISE_SHIFT_6 6
#define BITWISE_SHIFT_5 5
#define BITWISE_SHIFT_4 4
#define BITWISE_SHIFT_3 3
#define BITWISE_SHIFT_2 2
#define BITWISE_SHIFT_1 1

static uint8_t slope_test_done = 0;
static uint8_t motion_test_done = 0;


#ifdef CONFIG_KX022_DIAGNOSTIC_MODE
static void accel_cfg(const struct device *dev)
{
	int rc;
	uint8_t tmp;

	rc = kx022_read_register_value(dev, KX022_CNTL1, &tmp);
	printf("Operating mode %ld\n", (tmp & KX022_MASK_CNTL1_PC1) >> BITWISE_SHIFT_7);
	printf("Resolution %ld\n", (tmp & KX022_MASK_CNTL1_RES) >> BITWISE_SHIFT_6);
	printf("Acceleration range mode %ld\n", (tmp & KX022_MASK_CNTL1_GSEL) >> BITWISE_SHIFT_3);
	printf("Tap state %ld\n", (tmp & KX022_MASK_CNTL1_TDTE) >> BITWISE_SHIFT_2);
	printf("Motion detect state %ld\n", (tmp & KX022_MASK_CNTL1_WUFE) >> BITWISE_SHIFT_1);
	printf("Tilt state %ld\n", (tmp & KX022_MASK_CNTL1_TPE));

	rc = kx022_read_register_value(dev, KX022_ODCNTL, &tmp);
	printf("Accelerometer odr %ld\n", (tmp & KX022_MASK_ODCNTL_OSA));

	rc = kx022_read_register_value(dev, KX022_INC1, &tmp);
	printf("Pin 1 interrupt state %ld\n", (tmp & KX022_MASK_INC5_IEN2) >> BITWISE_SHIFT_5);
	printf("Pin 1 polarity %ld\n", (tmp & KX022_MASK_INC5_IEA2) >> BITWISE_SHIFT_4);

	rc = kx022_read_register_value(dev, KX022_INC4, &tmp);
	printf("Pin 1 tap report state %ld\n", (tmp & KX022_MASK_INC4_TDTI1) >> BITWISE_SHIFT_2); 
	printf("Pin 1 motion report state %ld\n", (tmp & KX022_MASK_INC4_WUFI1) >> BITWISE_SHIFT_1); 
	printf("Pin 1 tilt report state %ld\n", (tmp & KX022_MASK_INC4_TPI1)); 
}

static void accel_motion_cfg(const struct device *dev)
{
	int rc;
	uint8_t tmp;

	rc = kx022_read_register_value(dev, KX022_INC2, &tmp);
	printf("Motion detect xn direction %ld\n", (tmp & KX022_MASK_INC2_XNWUE) >> BITWISE_SHIFT_5);
	printf("Motion detect xp direction %ld\n", (tmp & KX022_MASK_INC2_XPWUE) >> BITWISE_SHIFT_4);
	printf("Motion detect yn direction %ld\n", (tmp & KX022_MASK_INC2_YNWUE) >> BITWISE_SHIFT_3);
	printf("Motion detect yp direction %ld\n", (tmp & KX022_MASK_INC2_YPWUE) >> BITWISE_SHIFT_2);
	printf("Motion detect zn direction %ld\n", (tmp & KX022_MASK_INC2_ZNWUE) >> BITWISE_SHIFT_1);
	printf("Motion detect zp direction %ld\n", (tmp & KX022_MASK_INC2_ZPWUE));

	rc = kx022_read_register_value(dev, KX022_CNTL3, &tmp);
	printf("Motion detect odr %ld\n", (tmp & KX022_MASK_CNTL3_OWUF));

	rc = kx022_read_register_value(dev, KX022_WUFC, &tmp);
	printf("Motion detection timer %d\n", tmp);

	rc = kx022_read_register_value(dev, KX022_ATH, &tmp);
	printf("Motion detect wake up threshold %d\n", tmp);
}

static void accel_tilt_cfg(const struct device *dev)
{
	int rc;
	uint8_t tmp;

	rc = kx022_read_register_value(dev, KX022_CNTL2, &tmp);
	printf("Tilt xn direction %ld\n", (tmp & KX022_MASK_CNTL2_LEM) >> BITWISE_SHIFT_5);
	printf("Tilt xp direction %ld\n", (tmp & KX022_MASK_CNTL2_RIM) >> BITWISE_SHIFT_4);
	printf("Tilt yn direction %ld\n", (tmp & KX022_MASK_CNTL2_DOM) >> BITWISE_SHIFT_3);
	printf("Tilt yp direction %ld\n", (tmp & KX022_MASK_CNTL2_UPM) >> BITWISE_SHIFT_2);
	printf("Tilt zn direction %ld\n", (tmp & KX022_MASK_CNTL2_FDM) >> BITWISE_SHIFT_1);
	printf("Tilt zp direction %ld\n", (tmp & KX022_MASK_CNTL2_FUM));

	rc = kx022_read_register_value(dev, KX022_CNTL3, &tmp);
	printf("Tilt odr %ld\n", (tmp & KX022_MASK_CNTL3_OTP) >> BITWISE_SHIFT_6);

	rc = kx022_read_register_value(dev, KX022_TILT_TIMER, &tmp);
	printf("Tilt timer %d\n", tmp);

	rc = kx022_read_register_value(dev, KX022_TILT_ANGLE_LL, &tmp);
	printf("Tilt angle ll %d\n", tmp);

	rc = kx022_read_register_value(dev, KX022_TILT_ANGLE_HL, &tmp);
	printf("Tilt angle hl %d\n", tmp);	
}
#endif  /*CONFIG_KX022_DIAGNOSTIC_MODE*/

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
	int rc = sensor_sample_fetch_chan(sensor,SENSOR_CHAN_ACCEL_XYZ);

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
	int rc = sensor_sample_fetch_chan(sensor, SENSOR_CHAN_KX022_MOTION);

	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_KX022_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor, SENSOR_CHAN_KX022_MOTION, rd_data);
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
	int rc = sensor_sample_fetch_chan(sensor,SENSOR_CHAN_KX022_TILT);

	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_KX022_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor, SENSOR_CHAN_KX022_TILT, rd_data);
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
static void motion_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	static unsigned int motion_cnt;

	fetch_and_display(dev);
	motion_display(dev);
	if (++motion_cnt > 5) {
		motion_test_done = 1;
		motion_cnt = 0;
	}
}
static void tilt_handler(const struct device *dev, const struct sensor_trigger *trig)
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

	trig.type = SENSOR_TRIG_KX022_MOTION;

	rc = sensor_trigger_set(dev, &trig, motion_handler);
	printf("\n");
	printf("\t\tAccelerometer: Motion  trigger test Start\r\n");

	while (motion_test_done == 0) {
		k_sleep(K_MSEC(200));
	}
	motion_test_done = 0;
	printf("\n");
	printf("\t\tAccelerometer: Motion trigger test finished\r\n");

	rc = kx022_restore_default_trigger_setup(dev, &trig);

	k_sleep(K_MSEC(500));

	trig.type = SENSOR_TRIG_KX022_TILT;

	rc = sensor_trigger_set(dev, &trig, tilt_handler);
	printf("\n");
	printf("\t\tAccelerometer: Tilt Position trigger test Start\r\n");

	while (slope_test_done == 0) {
		k_sleep(K_MSEC(200));
	}
	slope_test_done = 0;
	printf("\n");
	printf("\t\tAccelerometer: Tilt Position trigger test finished\r\n");

	rc = kx022_restore_default_trigger_setup(dev, &trig);
}

static void test_runtime_cfg(const struct device *dev)
{	
	//int rc;
	//struct sensor_value val;
#if CONFIG_KX022_FS_RUNTIME
	val.val1 = 1;
	rc = sensor_attr_set(dev, SENSOR_CHAN_KX022_CFG, SENSOR_ATTR_FULL_SCALE, &val);
#endif

#if CONFIG_KX022_ODR_RUNTIME
	val.val1 = 3;
	rc = sensor_attr_set(dev, SENSOR_CHAN_KX022_CFG, SENSOR_ATTR_KX022_ODR, &val);
#endif

#if CONFIG_KX022_RES_RUNTIME
	val.val1 = 0;
	rc = sensor_attr_set(dev, SENSOR_CHAN_KX022_CFG, SENSOR_ATTR_KX022_RESOLUTION, &val);
#endif

#if CONFIG_KX022_MOTION_DETECTION_TIMER_RUNTIME
	val.val1 = 2;
	rc = sensor_attr_set(dev, SENSOR_CHAN_KX022_CFG, SENSOR_ATTR_KX022_MOTION_DETECTION_TIMER, &val);
#endif

#if CONFIG_KX022_TILT_TIMER_RUNTIME
	val.val1 = 3;
	rc = sensor_attr_set(dev, SENSOR_CHAN_KX022_CFG, SENSOR_ATTR_KX022_TILT_TIMER, &val);
#endif

#if CONFIG_KX022_MOTION_DETECT_THRESHOLD_RUNTIME
	val.val1 = 4;
	rc = sensor_attr_set(dev, SENSOR_CHAN_KX022_CFG, SENSOR_ATTR_KX022_MOTION_DETECT_THRESHOLD, &val);
#endif

#if CONFIG_KX022_TILT_ANGLE_LL_RUNTIME
	val.val1 = 16;
	rc = sensor_attr_set(dev, SENSOR_CHAN_KX022_CFG, SENSOR_ATTR_KX022_TILT_ANGLE_LL, &val);
#endif
}

void main(void)
{
	const struct device *sensor = device_get_binding(DT_LABEL(DT_NODELABEL(kx022_device_1)));
	const struct device *sensor2 = device_get_binding(DT_LABEL(DT_NODELABEL(kx022_device_2)));

	if (!device_is_ready(sensor)) {
		printf("Device kx022 device 1 is not ready\n");
		sys_reboot(SYS_REBOOT_COLD);
		return;
	}

	if (!device_is_ready(sensor2)) {
		printf("Device kx022 device 2 is not ready\n");
		sys_reboot(SYS_REBOOT_COLD);
		return;
	}

#ifdef CONFIG_KX022_DIAGNOSTIC_MODE
	accel_cfg(sensor);
	accel_motion_cfg(sensor);
	accel_tilt_cfg(sensor);
#endif  /*CONFIG_KX022_DIAGNOSTIC_MODE*/

	test_runtime_cfg(sensor);
	
#ifdef CONFIG_KX022_DIAGNOSTIC_MODE
	printf("\nAfter cfg\n");
	accel_cfg(sensor);
	accel_motion_cfg(sensor);
	accel_tilt_cfg(sensor);
#endif  /*CONFIG_KX022_DIAGNOSTIC_MODE*/

	while (true) {
		test_polling_mode(sensor);
#ifdef CONFIG_KX022_TRIGGER
		test_trigger_mode(sensor);
#endif

		test_polling_mode(sensor2);
#ifdef CONFIG_KX022_TRIGGER
		test_trigger_mode(sensor2);
#endif

	}
}
