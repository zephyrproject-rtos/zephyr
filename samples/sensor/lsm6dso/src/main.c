/*
 * Copyright (c) 2020 Yestin Sun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#define LSM6DSO DT_INST(0, st_lsm6dso)

#if DT_NODE_HAS_STATUS(LSM6DSO, okay)
#define LSM6DSO_LABEL DT_LABEL(LSM6DSO)
#else
#error Your devicetree has no enabled nodes with compatible "st,lsm6dso"
#define LSM6DSO_LABEL "<none>"
#endif

static inline float out_ev(struct sensor_value *val)
{
	return (val->val1 + (float)val->val2 / 1000000);
}

static void fetch_and_display(const struct device *dev)
{
	struct sensor_value x, y, z;
	static int trig_cnt;

	trig_cnt++;

	/* lsm6dso accel */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &x);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &y);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &z);

	printf("accel x:%f ms/2 y:%f ms/2 z:%f ms/2\n",
			out_ev(&x), out_ev(&y), out_ev(&z));

	/* lsm6dso gyro */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_X, &x);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Y, &y);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Z, &z);

	printf("gyro x:%f dps y:%f dps z:%f dps\n",
			out_ev(&x), out_ev(&y), out_ev(&z));

	printf("trig_cnt:%d\n\n", trig_cnt);
}

static int set_sampling_freq(const struct device *dev)
{
	int ret = 0;
	struct sensor_value odr_attr;

	/* set accel/gyro sampling frequency to 12.5 Hz */
	odr_attr.val1 = 12.5;
	odr_attr.val2 = 0;

	ret = sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr);
	if (ret != 0) {
		printf("Cannot set sampling frequency for accelerometer.\n");
		return ret;
	}

	ret = sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr);
	if (ret != 0) {
		printf("Cannot set sampling frequency for gyro.\n");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_LSM6DSO_TRIGGER
static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{
	fetch_and_display(dev);
}

static void test_trigger_mode(const struct device *dev)
{
	struct sensor_trigger trig;

	if (set_sampling_freq(dev) != 0)
		return;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;

	if (sensor_trigger_set(dev, &trig, trigger_handler) != 0) {
		printf("Could not set sensor type and channel\n");
		return;
	}
}

#else
static void test_polling_mode(const struct device *dev)
{
	if (set_sampling_freq(dev) != 0)
		return;

	while (1) {
		fetch_and_display(dev);
		k_sleep(K_MSEC(1000));
	}
}
#endif

void main(void)
{
	const struct device *dev = device_get_binding(LSM6DSO_LABEL);

	if (dev == NULL) {
		printf("No device \"%s\" found.\n", LSM6DSO_LABEL);
		return;
	}

#ifdef CONFIG_LSM6DSO_TRIGGER
	printf("Testing LSM6DSO sensor in trigger mode.\n\n");
	test_trigger_mode(dev);
#else
	printf("Testing LSM6DSO sensor in polling mode.\n\n");
	test_polling_mode(dev);
#endif
}
