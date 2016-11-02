/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <stdio.h>
#include <device.h>
#include <sensor.h>
#include <misc/util.h>

#define MAX_TEST_TIME	15000
#define SLEEPTIME	300

/* uncomment next line for setting offsets manually */
/* #define PERFORM_MANUAL_CALIBRATION */

/* uncomment the next line for auto calibration */
/* #define PERFORM_AUTO_CALIBRATION */

#ifdef PERFORM_MANUAL_CALIBRATION
/*
 * Offset map needed for manual accelerometer calibration. These values are
 * deduced by holding the device perpendicular on one axis (usually that's Z if
 * the device lies flat on the table) and compute the difference so that the
 * values on the 3 axis look like this: X: 0, Y: 0; Z: 9.80665. Due to
 * accelerometer noise, the values will vary around these values.
 *
 * For example if the accelerometer output, without offset compensation, is :
 *
 *   Acc (m/s^2): X=-2.349435, Y=-0.488070, Z=11.158620
 *
 * then the offsets necessary to compensate the read values are:
 *   X = +2.349435, Y = +0.488070. Z = -1.351970
 */
static struct sensor_value accel_offsets[] = {
	{SENSOR_VALUE_TYPE_INT_PLUS_MICRO, { {2, 349435} } },   /* X */
	{SENSOR_VALUE_TYPE_INT_PLUS_MICRO, { {0, 488070} } },   /* Y */
	{SENSOR_VALUE_TYPE_INT_PLUS_MICRO, { {-1, -351970} } }, /* Z */
};

/*
 * The same goes for gyro offsets but, in gyro's case, the values should
 * converge to 0 (with the device standing still).
 */
static struct sensor_value gyro_offsets[] = {
	{SENSOR_VALUE_TYPE_INT_PLUS_MICRO, { {0, 3195} } }, /* X */
	{SENSOR_VALUE_TYPE_INT_PLUS_MICRO, { {0, 3195} } }, /* Y */
	{SENSOR_VALUE_TYPE_INT_PLUS_MICRO, { {0, -4260} } },/* Z */
};

static int manual_calibration(struct device *bmi160)
{
#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
	/* set accelerometer offsets */
	if (sensor_attr_set(bmi160, SENSOR_CHAN_ACCEL_ANY,
			    SENSOR_ATTR_OFFSET, accel_offsets) < 0) {
		return -EIO;
	}
#endif

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
	/* set gyroscope offsets */
	if (sensor_attr_set(bmi160, SENSOR_CHAN_GYRO_ANY,
			    SENSOR_ATTR_OFFSET, gyro_offsets) < 0) {
		return -EIO;
	}
#endif

	return 0;
}
#endif

#ifdef PERFORM_AUTO_CALIBRATION
/*
 * The values in the following map are the expected values that the
 * accelerometer needs to converge to if the device lies flat on the table. The
 * device has to stay still for about 500ms = 250ms(accel) + 250ms(gyro).
 */
struct sensor_value acc_calib[] = {
	{SENSOR_VALUE_TYPE_INT_PLUS_MICRO, { {0, 0} } },      /* X */
	{SENSOR_VALUE_TYPE_INT_PLUS_MICRO, { {0, 0} } },      /* Y */
	{SENSOR_VALUE_TYPE_INT_PLUS_MICRO, { {9, 806650} } }, /* Z */
};
static int auto_calibration(struct device *bmi160)
{
#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
	/* calibrate accelerometer */
	if (sensor_attr_set(bmi160, SENSOR_CHAN_ACCEL_ANY,
			      SENSOR_ATTR_CALIB_TARGET, acc_calib) < 0) {
		return -EIO;
	}
#endif

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
	/*
	 * Calibrate gyro. No calibration value needs to be passed to BMI160 as
	 * the target on all axis is set internally to 0. This is used just to
	 * trigger a gyro calibration.
	 */
	if (sensor_attr_set(bmi160, SENSOR_CHAN_GYRO_ANY,
			      SENSOR_ATTR_CALIB_TARGET, NULL) < 0) {
		return -EIO;
	}
#endif

	return 0;
}
#endif

/**
 * @brief Helper function for printing a sensor value to a buffer
 *
 * @param buf A pointer to the buffer to which the printing is done.
 * @param len Size of buffer in bytes.
 * @param val A pointer to a sensor_value struct holding the value
 *            to be printed.
 *
 * @return The number of characters printed to the buffer.
 */
static inline int sensor_value_snprintf(char *buf, size_t len,
					const struct sensor_value *val)
{
	int32_t val1, val2;

	switch (val->type) {
	case SENSOR_VALUE_TYPE_INT:
		return snprintf(buf, len, "%d", val->val1);
	case SENSOR_VALUE_TYPE_INT_PLUS_MICRO:
		if (val->val2 == 0) {
			return snprintf(buf, len, "%d", val->val1);
		}

		/* normalize value */
		if (val->val1 < 0 && val->val2 > 0) {
			val1 = val->val1 + 1;
			val2 = val->val2 - 1000000;
		} else {
			val1 = val->val1;
			val2 = val->val2;
		}

		/* print value to buffer */
		if (val1 > 0 || (val1 == 0 && val2 > 0)) {
			return snprintf(buf, len, "%d.%06d", val1, val2);
		} else if (val1 == 0 && val2 < 0) {
			return snprintf(buf, len, "-0.%06d", -val2);
		} else {
			return snprintf(buf, len, "%d.%06d", val1, -val2);
		}
	case SENSOR_VALUE_TYPE_DOUBLE:
		return snprintf(buf, len, "%f", val->dval);
	default:
		return 0;
	}
}

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
static void print_gyro_data(struct device *bmi160)
{
	struct sensor_value val[3];
	char buf_x[18], buf_y[18], buf_z[18];

	if (sensor_channel_get(bmi160, SENSOR_CHAN_GYRO_ANY, val) < 0) {
		printf("Cannot read bmi160 gyro channels.\n");
		return;
	}

	sensor_value_snprintf(buf_x, sizeof(buf_x), &val[0]);
	sensor_value_snprintf(buf_y, sizeof(buf_y), &val[1]);
	sensor_value_snprintf(buf_z, sizeof(buf_z), &val[2]);

	printf("Gyro (rad/s): X=%s, Y=%s, Z=%s\n", buf_x, buf_y, buf_z);
}
#endif

#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
static void print_accel_data(struct device *bmi160)
{
	struct sensor_value val[3];
	char buf_x[18], buf_y[18], buf_z[18];

	if (sensor_channel_get(bmi160,
			       SENSOR_CHAN_ACCEL_ANY, val) < 0) {
		printf("Cannot read bmi160 accel channels.\n");
		return;
	}

	sensor_value_snprintf(buf_x, sizeof(buf_x), &val[0]);
	sensor_value_snprintf(buf_y, sizeof(buf_y), &val[1]);
	sensor_value_snprintf(buf_z, sizeof(buf_z), &val[2]);

	printf("Acc (m/s^2): X=%s, Y=%s, Z=%s\n", buf_x, buf_y, buf_z);
}
#endif

static void print_temp_data(struct device *bmi160)
{
	struct sensor_value val;
	char buf[18];

	if (sensor_channel_get(bmi160, SENSOR_CHAN_TEMP, &val) < 0) {
		printf("Temperature channel read error.\n");
		return;
	}

	sensor_value_snprintf(buf, sizeof(buf), &val);

	printf("Temperature (Celsius): %s\n", buf);
}

static void test_polling_mode(struct device *bmi160)
{
	int32_t remaining_test_time = MAX_TEST_TIME;
	struct sensor_value attr;

#if defined(CONFIG_BMI160_ACCEL_ODR_RUNTIME)
	/* set sampling frequency to 800Hz for accel */
	attr.type = SENSOR_VALUE_TYPE_INT_PLUS_MICRO;
	attr.val1 = 800;
	attr.val2 = 0;

	if (sensor_attr_set(bmi160, SENSOR_CHAN_ACCEL_ANY,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &attr) < 0) {
		printf("Cannot set sampling frequency for accelerometer.\n");
		return;
	}
#endif

#if defined(CONFIG_BMI160_GYRO_ODR_RUNTIME)
	/* set sampling frequency to 3200Hz for gyro */
	attr.type = SENSOR_VALUE_TYPE_INT_PLUS_MICRO;
	attr.val1 = 3200;
	attr.val2 = 0;

	if (sensor_attr_set(bmi160, SENSOR_CHAN_GYRO_ANY,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &attr) < 0) {
		printf("Cannot set sampling frequency for gyroscope.\n");
		return;
	}
#endif

	/* poll the data and print it */
	do {
		if (sensor_sample_fetch(bmi160) < 0) {
			printf("Sample update error.\n");
			return;
		}

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
		print_gyro_data(bmi160);
#endif
#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
		print_accel_data(bmi160);
#endif
		print_temp_data(bmi160);

		/* wait a while */
		k_sleep(SLEEPTIME);

		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);
}

#ifdef CONFIG_BMI160_TRIGGER
static void trigger_hdlr(struct device *bmi160,
			 struct sensor_trigger *trigger)
{
	if (trigger->type != SENSOR_TRIG_DELTA &&
	    trigger->type != SENSOR_TRIG_DATA_READY) {
		return;
	}

	if (sensor_sample_fetch(bmi160) < 0) {
		printf("Sample update error.\n");
		return;
	}

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
	if (trigger->chan == SENSOR_CHAN_GYRO_ANY) {
		print_gyro_data(bmi160);
	}
#endif

#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
	if (trigger->chan == SENSOR_CHAN_ACCEL_ANY) {
		print_accel_data(bmi160);
	}
#endif
}

static void test_anymotion_trigger(struct device *bmi160)
{
	int32_t remaining_test_time = MAX_TEST_TIME;
	struct sensor_value attr;
	struct sensor_trigger trig;

	/* set up anymotion trigger */

	/*
	 * Set slope threshold to 0.1G (0.1 * 9.80665 = 4.903325 m/s^2).
	 * This depends on the chosen range. One cannot choose a threshold
	 * bigger than half the range. For example, for a 16G range, the
	 * threshold must not exceed 8G.
	 */
	attr.type = SENSOR_VALUE_TYPE_INT_PLUS_MICRO;
	attr.val1 = 0;
	attr.val2 = 980665;
	if (sensor_attr_set(bmi160, SENSOR_CHAN_ACCEL_ANY,
			    SENSOR_ATTR_SLOPE_TH, &attr) < 0) {
		printf("Cannot set anymotion slope threshold.\n");
		return;
	}

	/*
	 * Set slope duration to 2 consecutive samples (after which the
	 * anymotion interrupt will trigger.
	 *
	 * Allowed values are from 1 to 4.
	 */
	attr.type = SENSOR_VALUE_TYPE_INT;
	attr.val1 = 2;
	if (sensor_attr_set(bmi160, SENSOR_CHAN_ACCEL_ANY,
			    SENSOR_ATTR_SLOPE_DUR, &attr) < 0) {
		printf("Cannot set anymotion slope duration.\n");
		return;
	}

	/* enable anymotion trigger */
	trig.type = SENSOR_TRIG_DELTA;
	trig.chan = SENSOR_CHAN_ACCEL_ANY;

	if (sensor_trigger_set(bmi160, &trig, trigger_hdlr) < 0) {
		printf("Cannot enable anymotion trigger.\n");
		return;
	}

	printf("Anymotion test: shake the device to get anymotion events.\n");
	do {
		/* wait a while */
		k_sleep(SLEEPTIME);
		remaining_test_time -= SLEEPTIME;

	} while (remaining_test_time > 0);

	printf("Anymotion test: finished, removing anymotion trigger...\n");

	if (sensor_trigger_set(bmi160, &trig, NULL) < 0) {
		printf("Cannot remove anymotion trigger.\n");
		return;
	}
}

static void test_data_ready_trigger(struct device *bmi160)
{
	int32_t remaining_test_time = MAX_TEST_TIME;
	struct sensor_trigger trig;

	/* enable data ready trigger */
	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_ANY;

	if (sensor_trigger_set(bmi160, &trig, trigger_hdlr) < 0) {
		printf("Cannot enable data ready trigger.\n");
		return;
	}

	printf("Data ready test:\n");
	do {
		/* wait a while */
		k_sleep(SLEEPTIME);
		remaining_test_time -= SLEEPTIME;

	} while (remaining_test_time > 0);

	printf("Data ready test: finished, removing data ready trigger...\n");

	if (sensor_trigger_set(bmi160, &trig, NULL) < 0) {
		printf("Cannot remove data ready trigger.\n");
		return;
	}
}

static void test_trigger_mode(struct device *bmi160)
{
	struct sensor_value attr;

#if defined(CONFIG_BMI160_ACCEL_ODR_RUNTIME)
	/* set sampling frequency to 100Hz for accel */
	attr.type = SENSOR_VALUE_TYPE_INT_PLUS_MICRO;
	attr.val1 = 100;
	attr.val2 = 0;

	if (sensor_attr_set(bmi160, SENSOR_CHAN_ACCEL_ANY,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &attr) < 0) {
		printf("Cannot set sampling frequency for accelerometer.\n");
		return;
	}
#endif

#if defined(CONFIG_BMI160_GYRO_ODR_RUNTIME)
	/* set sampling frequency to 100Hz for gyro */
	attr.type = SENSOR_VALUE_TYPE_INT_PLUS_MICRO;
	attr.val1 = 100;
	attr.val2 = 0;

	if (sensor_attr_set(bmi160, SENSOR_CHAN_GYRO_ANY,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &attr) < 0) {
		printf("Cannot set sampling frequency for gyroscope.\n");
		return;
	}
#endif

	test_anymotion_trigger(bmi160);

	test_data_ready_trigger(bmi160);
}
#endif /* CONFIG_BMI160_TRIGGER */

extern uint8_t pbuf[1024];
extern uint8_t *pos;
void main(void)
{
	struct device *bmi160;
#if defined(CONFIG_BMI160_ACCEL_RANGE_RUNTIME) ||\
		defined(CONFIG_BMI160_GYRO_RANGE_RUNTIME)
	struct sensor_value attr;
#endif

	printf("IMU: Binding...\n");
	bmi160 = device_get_binding("bmi160");
	if (!bmi160) {
		printf("Gyro: Device not found.\n");
		return;
	}

#if defined(CONFIG_BMI160_ACCEL_RANGE_RUNTIME)
	/*
	 * Set accelerometer range to +/- 16G. Since the sensor API needs SI
	 * units, convert the range to m/s^2.
	 */
	sensor_g_to_ms2(16, &attr);

	if (sensor_attr_set(bmi160, SENSOR_CHAN_ACCEL_ANY,
			    SENSOR_ATTR_FULL_SCALE, &attr) < 0) {
		printf("Cannot set accelerometer range.\n");
		return;
	}
#endif

#if defined(CONFIG_BMI160_GYRO_RANGE_RUNTIME)
	/*
	 * Set gyro range to +/- 250 degrees/s. Since the sensor API needs SI
	 * units, convert the range to rad/s.
	 */
	sensor_degrees_to_rad(250, &attr);

	if (sensor_attr_set(bmi160, SENSOR_CHAN_GYRO_ANY,
			    SENSOR_ATTR_FULL_SCALE, &attr) < 0) {
		printf("Cannot set gyro range.\n");
		return;
	}
#endif

#ifdef PERFORM_MANUAL_CALIBRATION
	/* manually adjust accelerometer and gyro offsets */
	if (manual_calibration(bmi160) < 0) {
		printf("Manual calibration failed.\n");
		return;
	}
#endif

#ifdef PERFORM_AUTO_CALIBRATION
	/* auto calibrate accelerometer and gyro */
	if (auto_calibration(bmi160) < 0) {
		printf("HW calibration failed.\n");
		return;
	}
#endif

	printf("Testing the polling mode.\n");
	test_polling_mode(bmi160);
	printf("Testing the polling mode finished.\n");

#ifdef CONFIG_BMI160_TRIGGER
	printf("Testing the trigger mode.\n");
	test_trigger_mode(bmi160);
	printf("Testing the trigger mode finished.\n");
#endif
}
