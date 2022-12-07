/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_LIS2DW12_TRIGGER
static int lis2dw12_trig_cnt;

static void lis2dw12_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	lis2dw12_trig_cnt++;
}
#endif

#ifdef CONFIG_LSM6DSO_TRIGGER
static int lsm6dso_acc_trig_cnt;
static int lsm6dso_gyr_trig_cnt;
static int lsm6dso_temp_trig_cnt;

static void lsm6dso_acc_trig_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	lsm6dso_acc_trig_cnt++;
}

static void lsm6dso_gyr_trig_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	lsm6dso_gyr_trig_cnt++;
}

static void lsm6dso_temp_trig_handler(const struct device *dev,
				      const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_DIE_TEMP);
	lsm6dso_temp_trig_cnt++;
}
#endif

static void lis2dw12_config(const struct device *lis2dw12)
{
	struct sensor_value odr_attr, fs_attr;

	/* set LIS2DW12 accel/gyro sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lis2dw12, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LIS2DW12 accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(lis2dw12, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set sampling frequency for LIS2DW12 gyro\n");
		return;
	}

#ifdef CONFIG_LIS2DW12_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lis2dw12, &trig, lis2dw12_trigger_handler);
#endif
}

static void lsm6dso_config(const struct device *lsm6dso)
{
	struct sensor_value odr_attr, fs_attr;

	/* set LSM6DSO accel sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for LSM6DSO accel\n");
		return;
	}

	/* set LSM6DSO gyro sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO gyro\n");
		return;
	}

	sensor_degrees_to_rad(250, &fs_attr);

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for LSM6DSO gyro\n");
		return;
	}

	/* set LSM6DSO external magn sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

#ifdef CONFIG_LSM6DSO_EXT_LIS2MDL
	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_MAGN_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO ext magn\n");
		return;
	}
#endif

#ifdef CONFIG_LSM6DSO_EXT_LPS22HH
	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_PRESS,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO ext pressure\n");
		return;
	}
#endif

#ifdef CONFIG_LSM6DSO_EXT_HTS221
	odr_attr.val1 = 12;
	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_HUMIDITY,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO ext humidity\n");
		return;
	}
#endif

#ifdef CONFIG_LSM6DSO_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lsm6dso, &trig, lsm6dso_acc_trig_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;
	sensor_trigger_set(lsm6dso, &trig, lsm6dso_gyr_trig_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_DIE_TEMP;
	sensor_trigger_set(lsm6dso, &trig, lsm6dso_temp_trig_handler);
#endif
}

void main(void)
{
#ifdef CONFIG_LSM6DSO_EXT_LPS22HH
	struct sensor_value temp2, press;
#endif
#ifdef CONFIG_LSM6DSO_EXT_HTS221
	struct sensor_value hum;
#endif
#ifdef CONFIG_LSM6DSO_ENABLE_TEMP
	struct sensor_value die_temp;
#endif
	struct sensor_value accel1[3], accel2[3];
	struct sensor_value gyro[3];
	struct sensor_value magn[3];
	const struct device *const lis2dw12 = DEVICE_DT_GET_ONE(st_lis2dw12);
	const struct device *const lsm6dso = DEVICE_DT_GET_ONE(st_lsm6dso);
	int cnt = 1;

	if (!device_is_ready(lis2dw12)) {
		printk("%s: device not ready.\n", lis2dw12->name);
		return;
	}
	if (!device_is_ready(lsm6dso)) {
		printk("%s: device not ready.\n", lsm6dso->name);
		return;
	}

	lis2dw12_config(lis2dw12);
	lsm6dso_config(lsm6dso);

	while (1) {
		/* Get sensor samples */

#ifndef CONFIG_LIS2DW12_TRIGGER
		if (sensor_sample_fetch(lis2dw12) < 0) {
			printf("LIS2DW12 Sensor sample update error\n");
			return;
		}
#endif
#ifndef CONFIG_LSM6DSO_TRIGGER
		if (sensor_sample_fetch(lsm6dso) < 0) {
			printf("LSM6DSO Sensor sample update error\n");
			return;
		}
#endif

		/* Get sensor data */
		sensor_channel_get(lis2dw12, SENSOR_CHAN_ACCEL_XYZ, accel2);
		sensor_channel_get(lsm6dso, SENSOR_CHAN_ACCEL_XYZ, accel1);
		sensor_channel_get(lsm6dso, SENSOR_CHAN_GYRO_XYZ, gyro);
#ifdef CONFIG_LSM6DSO_ENABLE_TEMP
		sensor_channel_get(lsm6dso, SENSOR_CHAN_DIE_TEMP, &die_temp);
#endif
#ifdef CONFIG_LSM6DSO_EXT_LIS2MDL
		sensor_channel_get(lsm6dso, SENSOR_CHAN_MAGN_XYZ, magn);
#endif
#ifdef CONFIG_LSM6DSO_EXT_LPS22HH
		sensor_channel_get(lsm6dso, SENSOR_CHAN_AMBIENT_TEMP, &temp2);
		sensor_channel_get(lsm6dso, SENSOR_CHAN_PRESS, &press);
#endif
#ifdef CONFIG_LSM6DSO_EXT_HTS221
		sensor_channel_get(lsm6dso, SENSOR_CHAN_HUMIDITY, &hum);
#endif

		/* Display sensor data */

		/* Erase previous */
		printf("\0033\014");

		printf("X-NUCLEO-IKS01A3 sensor dashboard\n\n");

		printf("LIS2DW12: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&accel2[0]),
			sensor_value_to_double(&accel2[1]),
			sensor_value_to_double(&accel2[2]));

		printf("LSM6DSO: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&accel1[0]),
			sensor_value_to_double(&accel1[1]),
			sensor_value_to_double(&accel1[2]));

		printf("LSM6DSO: GYro (dps): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&gyro[0]),
			sensor_value_to_double(&gyro[1]),
			sensor_value_to_double(&gyro[2]));

#ifdef CONFIG_LSM6DSO_ENABLE_TEMP
		/* temperature */
		printf("LSM6DSO: Temperature: %.1f C\n",
		       sensor_value_to_double(&die_temp));
#endif

#ifdef CONFIG_LSM6DSO_EXT_LIS2MDL
		printf("LSM6DSO: Magn (gauss): x: %.3f, y: %.3f, z: %.3f\n",
		       sensor_value_to_double(&magn[0]),
		       sensor_value_to_double(&magn[1]),
		       sensor_value_to_double(&magn[2]));
#endif

#ifdef CONFIG_LSM6DSO_EXT_LPS22HH
		printf("LSM6DSO: Temperature: %.1f C\n",
		       sensor_value_to_double(&temp2));

		printf("LSM6DSO: Pressure:%.3f kpa\n",
		       sensor_value_to_double(&press));
#endif

#ifdef CONFIG_LSM6DSO_EXT_HTS221
		printf("LSM6DSO: Relative Humidity: %.1f%%\n",
		       sensor_value_to_double(&hum));

#endif

#ifdef CONFIG_LIS2DW12_TRIGGER
		printk("%d:: lis2dw12 trig %d\n", cnt, lis2dw12_trig_cnt);
#endif

#ifdef CONFIG_LSM6DSO_TRIGGER
		printk("%d:: lsm6dso acc trig %d\n", cnt, lsm6dso_acc_trig_cnt);
		printk("%d:: lsm6dso gyr trig %d\n", cnt, lsm6dso_gyr_trig_cnt);
		printk("%d:: lsm6dso temp trig %d\n", cnt,
			lsm6dso_temp_trig_cnt);
#endif

		cnt++;
		k_sleep(K_MSEC(2000));
	}
}
