/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <sys/util.h>

#ifdef CONFIG_LIS2MDL_TRIGGER
static int lis2mdl_trig_cnt;

static void lis2mdl_trigger_handler(const struct device *dev,
				    struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	lis2mdl_trig_cnt++;
}
#endif

#ifdef CONFIG_LPS22HH_TRIGGER
static int lps22hh_trig_cnt;

static void lps22hh_trigger_handler(const struct device *dev,
				    struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS);
	lps22hh_trig_cnt++;
}
#endif

#ifdef CONFIG_STTS751_TRIGGER
static int stts751_trig_cnt;

static void stts751_trigger_handler(const struct device *dev,
				       struct sensor_trigger *trig)
{
	stts751_trig_cnt++;
}
#endif

#ifdef CONFIG_LIS2DW12_TRIGGER
static int lis2dw12_trig_cnt;

static void lis2dw12_trigger_handler(const struct device *dev,
				     struct sensor_trigger *trig)
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
				     struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	lsm6dso_acc_trig_cnt++;
}

static void lsm6dso_gyr_trig_handler(const struct device *dev,
				     struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	lsm6dso_gyr_trig_cnt++;
}

static void lsm6dso_temp_trig_handler(const struct device *dev,
				      struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_DIE_TEMP);
	lsm6dso_temp_trig_cnt++;
}
#endif

static void lis2mdl_config(const struct device *lis2mdl)
{
	struct sensor_value odr_attr;

	/* set LIS2MDL sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lis2mdl, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LIS2MDL\n");
		return;
	}

#ifdef CONFIG_LIS2MDL_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_MAGN_XYZ;
	sensor_trigger_set(lis2mdl, &trig, lis2mdl_trigger_handler);
#endif
}

static void lps22hh_config(const struct device *lps22hh)
{
	struct sensor_value odr_attr;

	/* set LPS22HH sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lps22hh, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LPS22HH\n");
		return;
	}

#ifdef CONFIG_LPS22HH_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ALL;
	sensor_trigger_set(lps22hh, &trig, lps22hh_trigger_handler);
#endif
}

static void stts751_config(const struct device *stts751)
{
	struct sensor_value odr_attr;

	/* set STTS751 conversion rate to 16 Hz */
	odr_attr.val1 = 16;
	odr_attr.val2 = 0;

	if (sensor_attr_set(stts751, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for STTS751\n");
		return;
	}

#ifdef CONFIG_STTS751_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_THRESHOLD;
	trig.chan = SENSOR_CHAN_ALL;
	sensor_trigger_set(stts751, &trig, stts751_trigger_handler);
#endif
}

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
	struct sensor_value temp1, temp2, temp3, hum, press;
#ifdef CONFIG_LSM6DSO_ENABLE_TEMP
	struct sensor_value die_temp;
#endif
	struct sensor_value die_temp2;
	struct sensor_value accel1[3], accel2[3];
	struct sensor_value gyro[3];
	struct sensor_value magn[3];
	const struct device *hts221 = device_get_binding(DT_LABEL(DT_INST(0, st_hts221)));
	const struct device *lps22hh = device_get_binding(DT_LABEL(DT_INST(0, st_lps22hh)));
	const struct device *stts751 = device_get_binding(DT_LABEL(DT_INST(0, st_stts751)));
	const struct device *lis2mdl = device_get_binding(DT_LABEL(DT_INST(0, st_lis2mdl)));
	const struct device *lis2dw12 = device_get_binding(DT_LABEL(DT_INST(0, st_lis2dw12)));
	const struct device *lsm6dso = device_get_binding(DT_LABEL(DT_INST(0, st_lsm6dso)));
	int cnt = 1;

	if (hts221 == NULL) {
		printf("Could not get HTS221 device\n");
		return;
	}
	if (lps22hh == NULL) {
		printf("Could not get LPS22HH device\n");
		return;
	}
	if (stts751 == NULL) {
		printf("Could not get STTS751 device\n");
		return;
	}
	if (lis2mdl == NULL) {
		printf("Could not get LIS2MDL Magn device\n");
		return;
	}
	if (lis2dw12 == NULL) {
		printf("Could not get LIS2DW12 device\n");
		return;
	}
	if (lsm6dso == NULL) {
		printf("Could not get LSM6DSO device\n");
		return;
	}

	lis2mdl_config(lis2mdl);
	lps22hh_config(lps22hh);
	stts751_config(stts751);
	lis2dw12_config(lis2dw12);
	lsm6dso_config(lsm6dso);

	while (1) {
		/* Get sensor samples */

		if (sensor_sample_fetch(hts221) < 0) {
			printf("HTS221 Sensor sample update error\n");
			return;
		}
#ifndef CONFIG_LPS22HH_TRIGGER
		if (sensor_sample_fetch(lps22hh) < 0) {
			printf("LPS22HH Sensor sample update error\n");
			return;
		}
#endif
		if (sensor_sample_fetch(stts751) < 0) {
			printf("STTS751 Sensor sample update error\n");
			return;
		}

#ifndef CONFIG_LIS2MDL_TRIGGER
		if (sensor_sample_fetch(lis2mdl) < 0) {
			printf("LIS2MDL Magn Sensor sample update error\n");
			return;
		}
#endif

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

		sensor_channel_get(hts221, SENSOR_CHAN_AMBIENT_TEMP, &temp1);
		sensor_channel_get(hts221, SENSOR_CHAN_HUMIDITY, &hum);
		sensor_channel_get(lps22hh, SENSOR_CHAN_AMBIENT_TEMP, &temp2);
		sensor_channel_get(lps22hh, SENSOR_CHAN_PRESS, &press);
		sensor_channel_get(stts751, SENSOR_CHAN_AMBIENT_TEMP, &temp3);
		sensor_channel_get(lis2mdl, SENSOR_CHAN_MAGN_XYZ, magn);
		sensor_channel_get(lis2mdl, SENSOR_CHAN_DIE_TEMP, &die_temp2);
		sensor_channel_get(lis2dw12, SENSOR_CHAN_ACCEL_XYZ, accel2);
		sensor_channel_get(lsm6dso, SENSOR_CHAN_ACCEL_XYZ, accel1);
		sensor_channel_get(lsm6dso, SENSOR_CHAN_GYRO_XYZ, gyro);
#ifdef CONFIG_LSM6DSO_ENABLE_TEMP
		sensor_channel_get(lsm6dso, SENSOR_CHAN_DIE_TEMP, &die_temp);
#endif

		/* Display sensor data */

		/* Erase previous */
		printf("\0033\014");

		printf("X-NUCLEO-IKS01A3 sensor dashboard\n\n");

		/* temperature */
		printf("HTS221: Temperature: %.1f C\n",
		       sensor_value_to_double(&temp1));

		/* humidity */
		printf("HTS221: Relative Humidity: %.1f%%\n",
		       sensor_value_to_double(&hum));

		/* temperature */
		printf("LPS22HH: Temperature: %.1f C\n",
		       sensor_value_to_double(&temp2));

		/* pressure */
		printf("LPS22HH: Pressure:%.3f kpa\n",
		       sensor_value_to_double(&press));

		/* temperature */
		printf("STTS751: Temperature: %.1f C\n",
		       sensor_value_to_double(&temp3));

		/* lis2mdl */
		printf("LIS2MDL: Magn (gauss): x: %.3f, y: %.3f, z: %.3f\n",
		       sensor_value_to_double(&magn[0]),
		       sensor_value_to_double(&magn[1]),
		       sensor_value_to_double(&magn[2]));

		printf("LIS2MDL: Temperature: %.1f C\n",
		       sensor_value_to_double(&die_temp2));

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

#if defined(CONFIG_LIS2MDL_TRIGGER)
		printk("%d:: lis2mdl trig %d\n", cnt, lis2mdl_trig_cnt);
#endif

#if defined(CONFIG_LPS22HH_TRIGGER)
		printk("%d:: lps22hh trig %d\n", cnt, lps22hh_trig_cnt);
#endif

#if defined(CONFIG_STTS751_TRIGGER)
		printk("%d:: stts751 trig %d\n", cnt, stts751_trig_cnt);
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
