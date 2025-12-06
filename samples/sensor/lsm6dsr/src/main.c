/*
 * Copyright (c) 2025 Helbling Technik AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

static int print_samples;
static int lsm6dsr_trig_cnt;

static struct sensor_value accel_x_out, accel_y_out, accel_z_out;
static struct sensor_value gyro_x_out, gyro_y_out, gyro_z_out;

#ifdef CONFIG_LSM6DSR_TRIGGER
static void lsm6dsr_trigger_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	static struct sensor_value accel_x, accel_y, accel_z;
	static struct sensor_value gyro_x, gyro_y, gyro_z;

	lsm6dsr_trig_cnt++;

	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &accel_x);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &accel_y);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &accel_z);

	/* lsm6dsr gyro */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_X, &gyro_x);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Y, &gyro_y);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Z, &gyro_z);

	if (print_samples) {
		print_samples = 0;

		accel_x_out = accel_x;
		accel_y_out = accel_y;
		accel_z_out = accel_z;

		gyro_x_out = gyro_x;
		gyro_y_out = gyro_y;
		gyro_z_out = gyro_z;
	}
}
#endif

int main(void)
{
	int cnt = 0;
	char out_str[64];
	struct sensor_value odr_attr;
	const struct device *const lsm6dsr_dev = DEVICE_DT_GET_ONE(st_lsm6dsr);

	if (!device_is_ready(lsm6dsr_dev)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	/* set accel/gyro sampling frequency to 104 Hz */
	odr_attr.val1 = 104;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dsr_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
			    &odr_attr) < 0) {
		printk("Cannot set sampling frequency for accelerometer.\n");
		return 0;
	}

	if (sensor_attr_set(lsm6dsr_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY,
			    &odr_attr) < 0) {
		printk("Cannot set sampling frequency for gyro.\n");
		return 0;
	}

#ifdef CONFIG_LSM6DSR_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;

	if (sensor_trigger_set(lsm6dsr_dev, &trig, lsm6dsr_trigger_handler) != 0) {
		printk("Could not set sensor type and channel\n");
		return 0;
	}
#endif

	if (sensor_sample_fetch(lsm6dsr_dev) < 0) {
		printk("Sensor sample update error\n");
		return 0;
	}

	while (1) {
		/* Erase previous */
		printk("\0033\014");
		printf("LSM6DSR sensor samples:\n\n");

		/* lsm6dsr accel */
		sprintf(out_str, "accel x:%f ms/2 y:%f ms/2 z:%f ms/2",
			sensor_value_to_double(&accel_x_out), sensor_value_to_double(&accel_y_out),
			sensor_value_to_double(&accel_z_out));
		printk("%s\n", out_str);

		/* lsm6dsr gyro */
		sprintf(out_str, "gyro x:%f dps y:%f dps z:%f dps",
			sensor_value_to_double(&gyro_x_out), sensor_value_to_double(&gyro_y_out),
			sensor_value_to_double(&gyro_z_out));
		printk("%s\n", out_str);

		printk("loop:%d trig_cnt:%d\n\n", ++cnt, lsm6dsr_trig_cnt);

		print_samples = 1;
		k_sleep(K_MSEC(1000));
	}
}
