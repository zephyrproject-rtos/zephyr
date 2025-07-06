/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_LPS2XDF_TRIGGER
static int lps22df_trig_cnt;

static void lps22df_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	lps22df_trig_cnt++;
}
#endif

#ifdef CONFIG_LIS2MDL_TRIGGER
static int lis2mdl_trig_cnt;

static void lis2mdl_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	lis2mdl_trig_cnt++;
}
#endif

#ifdef CONFIG_LSM6DSO16IS_TRIGGER
static int lsm6dso16is_acc_trig_cnt;

static void lsm6dso16is_acc_trig_handler(const struct device *dev,
					 const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	lsm6dso16is_acc_trig_cnt++;
}
#endif

#ifdef CONFIG_LSM6DSV16X_TRIGGER
static int lsm6dsv16x_acc_trig_cnt;

static void lsm6dsv16x_acc_trig_handler(const struct device *dev,
					const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	lsm6dsv16x_acc_trig_cnt++;
}
#endif

#ifdef CONFIG_LIS2DUX12_TRIGGER
static int lis2duxs12_acc_trig_cnt;

static void lis2duxs12_acc_trig_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	lis2duxs12_acc_trig_cnt++;
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

static void lsm6dso16is_config(const struct device *lsm6dso16is)
{
	struct sensor_value odr_attr, fs_attr, mode_attr;

	mode_attr.val1 = 0; /* HP */

	if (sensor_attr_set(lsm6dso16is, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_CONFIGURATION, &mode_attr) < 0) {
		printk("Cannot set mode for LSM6DSO16IS accel\n");
		return;
	}

	/* set LSM6DSO16IS accel sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dso16is, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO16IS accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(lsm6dso16is, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set full scale for LSM6DSO16IS accel\n");
		return;
	}

	/* set LSM6DSO16IS gyro sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dso16is, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO16IS gyro\n");
		return;
	}

	sensor_degrees_to_rad(250, &fs_attr);

	if (sensor_attr_set(lsm6dso16is, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set full scale for LSM6DSO16IS gyro\n");
		return;
	}

#ifdef CONFIG_LSM6DSO16IS_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lsm6dso16is, &trig, lsm6dso16is_acc_trig_handler);
#endif
}

static void lsm6dsv16x_config(const struct device *lsm6dsv16x)
{
	struct sensor_value odr_attr, fs_attr, mode_attr;

	mode_attr.val1 = 0; /* HP */

	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_CONFIGURATION, &mode_attr) < 0) {
		printk("Cannot set mode for LSM6DSV16X accel\n");
		return;
	}

	/* set LSM6DSV16X accel sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSV16X accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set full scale for LSM6DSV16X accel\n");
		return;
	}

	/* set LSM6DSV16X gyro sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSV16X gyro\n");
		return;
	}

	sensor_degrees_to_rad(250, &fs_attr);

	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set full scale for LSM6DSV16X gyro\n");
		return;
	}

#ifdef CONFIG_LSM6DSV16X_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lsm6dsv16x, &trig, lsm6dsv16x_acc_trig_handler);
#endif
}

static void lps22df_config(const struct device *lps22df)
{
	struct sensor_value odr_attr;

	/* set LPS22DF accel sampling frequency to 10 Hz */
	odr_attr.val1 = 10;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lps22df, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LPS22DF accel\n");
		return;
	}

#ifdef CONFIG_LPS2XDF_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ALL;
	sensor_trigger_set(lps22df, &trig, lps22df_trigger_handler);
#endif
}

static void lis2duxs12_config(const struct device *lis2duxs12)
{
	struct sensor_value odr_attr, fs_attr;

	/* set LSM6DSV16X accel sampling frequency to 200 Hz */
	odr_attr.val1 = 200;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lis2duxs12, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSV16X accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(lis2duxs12, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for LSM6DSV16X accel\n");
		return;
	}

#ifdef CONFIG_LIS2DUX12_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lis2duxs12, &trig, lis2duxs12_acc_trig_handler);
#endif
}

int main(void)
{
	struct sensor_value lis2mdl_magn[3], lis2mdl_temp, lps22df_press, lps22df_temp;
	struct sensor_value lsm6dso16is_xl[3], lsm6dso16is_gy[3];
#ifdef CONFIG_LSM6DSO16IS_ENABLE_TEMP
	struct sensor_value lsm6dso16is_temp;
#endif
#ifdef CONFIG_LSM6DSV16X_ENABLE_TEMP
	struct sensor_value lsm6dsv16x_temp;
#endif
	struct sensor_value lsm6dsv16x_xl[3], lsm6dsv16x_gy[3];
	struct sensor_value lis2duxs12_xl[3], lis2duxs12_temp;
	const struct device *const lis2mdl = DEVICE_DT_GET_ONE(st_lis2mdl);
	const struct device *const lsm6dso16is = DEVICE_DT_GET_ONE(st_lsm6dso16is);
	const struct device *const lsm6dsv16x = DEVICE_DT_GET_ONE(st_lsm6dsv16x);
	const struct device *const lps22df = DEVICE_DT_GET_ONE(st_lps22df);
	const struct device *const lis2duxs12 = DEVICE_DT_GET_ONE(st_lis2duxs12);
	int cnt = 1;

	if (!device_is_ready(lsm6dso16is)) {
		printk("%s: device not ready.\n", lsm6dso16is->name);
		return 0;
	}
	if (!device_is_ready(lsm6dsv16x)) {
		printk("%s: device not ready.\n", lsm6dsv16x->name);
		return 0;
	}
	if (!device_is_ready(lis2mdl)) {
		printk("%s: device not ready.\n", lis2mdl->name);
		return 0;
	}
	if (!device_is_ready(lps22df)) {
		printk("%s: device not ready.\n", lps22df->name);
		return 0;
	}
	if (!device_is_ready(lis2duxs12)) {
		printk("%s: device not ready.\n", lis2duxs12->name);
		return 0;
	}

	lis2mdl_config(lis2mdl);
	lsm6dso16is_config(lsm6dso16is);
	lsm6dsv16x_config(lsm6dsv16x);
	lps22df_config(lps22df);
	lis2duxs12_config(lis2duxs12);

	while (1) {
		/* Get sensor samples */

#ifndef CONFIG_LIS2MDL_TRIGGER
		if (sensor_sample_fetch(lis2mdl) < 0) {
			printf("LIS2MDL Magn Sensor sample update error\n");
			return 0;
		}
#endif
#ifndef CONFIG_LSM6DSO16IS_TRIGGER
		if (sensor_sample_fetch(lsm6dso16is) < 0) {
			printf("LSM6DSO16IS Sensor sample update error\n");
			return 0;
		}
#endif
#ifndef CONFIG_LSM6DSV16X_TRIGGER
		if (sensor_sample_fetch(lsm6dsv16x) < 0) {
			printf("LSM6DSV16X Sensor sample update error\n");
			return 0;
		}
#endif
#ifndef CONFIG_LPS2XDF_TRIGGER
		if (sensor_sample_fetch(lps22df) < 0) {
			printf("LPS22DF pressure sample update error\n");
			return 0;
		}
#endif
#ifndef CONFIG_LIS2DUX12_TRIGGER
		if (sensor_sample_fetch(lis2duxs12) < 0) {
			printf("LIS2DUXS12 XL Sensor sample update error\n");
			return 0;
		}
#endif

		/* Get sensor data */
		sensor_channel_get(lis2mdl, SENSOR_CHAN_MAGN_XYZ, lis2mdl_magn);
		sensor_channel_get(lis2mdl, SENSOR_CHAN_DIE_TEMP, &lis2mdl_temp);
		sensor_channel_get(lsm6dso16is, SENSOR_CHAN_ACCEL_XYZ, lsm6dso16is_xl);
		sensor_channel_get(lsm6dso16is, SENSOR_CHAN_GYRO_XYZ, lsm6dso16is_gy);
#ifdef CONFIG_LSM6DSO16IS_ENABLE_TEMP
		sensor_channel_get(lsm6dso16is, SENSOR_CHAN_DIE_TEMP, &lsm6dso16is_temp);
#endif
#ifdef CONFIG_LSM6DSV16X_ENABLE_TEMP
		sensor_channel_get(lsm6dsv16x, SENSOR_CHAN_DIE_TEMP, &lsm6dsv16x_temp);
#endif
		sensor_channel_get(lsm6dsv16x, SENSOR_CHAN_ACCEL_XYZ, lsm6dsv16x_xl);
		sensor_channel_get(lsm6dsv16x, SENSOR_CHAN_GYRO_XYZ, lsm6dsv16x_gy);

		sensor_channel_get(lps22df, SENSOR_CHAN_PRESS, &lps22df_press);
		sensor_channel_get(lps22df, SENSOR_CHAN_AMBIENT_TEMP, &lps22df_temp);

		sensor_channel_get(lis2duxs12, SENSOR_CHAN_ACCEL_XYZ, lis2duxs12_xl);
		sensor_channel_get(lis2duxs12, SENSOR_CHAN_DIE_TEMP, &lis2duxs12_temp);
		/* Display sensor data */

		/* Erase previous */
		printf("\0033\014");

		printf("X-NUCLEO-IKS4A1 sensor dashboard\n\n");

		/* lis2mdl */
		printf("LIS2MDL: Magn (gauss): x: %.3f, y: %.3f, z: %.3f\n",
		       sensor_value_to_double(&lis2mdl_magn[0]),
		       sensor_value_to_double(&lis2mdl_magn[1]),
		       sensor_value_to_double(&lis2mdl_magn[2]));

		printf("LIS2MDL: Temperature: %.1f C\n",
		       sensor_value_to_double(&lis2mdl_temp));

		printf("LSM6DSO16IS: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lsm6dso16is_xl[0]),
			sensor_value_to_double(&lsm6dso16is_xl[1]),
			sensor_value_to_double(&lsm6dso16is_xl[2]));

		printf("LSM6DSO16IS: Gyro (dps): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lsm6dso16is_gy[0]),
			sensor_value_to_double(&lsm6dso16is_gy[1]),
			sensor_value_to_double(&lsm6dso16is_gy[2]));

#ifdef CONFIG_LSM6DSO16IS_ENABLE_TEMP
		/* temperature */
		printf("LSM6DSO16IS: Temperature: %.1f C\n",
		       sensor_value_to_double(&lsm6dso16is_temp));
#endif

		printf("LSM6DSV16X: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lsm6dsv16x_xl[0]),
			sensor_value_to_double(&lsm6dsv16x_xl[1]),
			sensor_value_to_double(&lsm6dsv16x_xl[2]));

		printf("LSM6DSV16X: GYro (dps): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lsm6dsv16x_gy[0]),
			sensor_value_to_double(&lsm6dsv16x_gy[1]),
			sensor_value_to_double(&lsm6dsv16x_gy[2]));

#ifdef CONFIG_LSM6DSV16X_ENABLE_TEMP
		/* temperature */
		printf("LSM6DSV16X: Temperature: %.1f C\n",
		       sensor_value_to_double(&lsm6dsv16x_temp));
#endif

		printf("LPS22DF: Temperature: %.1f C\n", sensor_value_to_double(&lps22df_temp));
		printf("LPS22DF: Pressure:%.3f kpa\n", sensor_value_to_double(&lps22df_press));

		printf("LIS2DUXS12: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lis2duxs12_xl[0]),
			sensor_value_to_double(&lis2duxs12_xl[1]),
			sensor_value_to_double(&lis2duxs12_xl[2]));

		printf("LIS2DUXS12: Temperature: %.1f C\n",
		       sensor_value_to_double(&lis2duxs12_temp));

#if defined(CONFIG_LIS2MDL_TRIGGER)
		printk("%d: lis2mdl trig %d\n", cnt, lis2mdl_trig_cnt);
#endif

#ifdef CONFIG_LSM6DSO16IS_TRIGGER
		printk("%d: lsm6dso16is acc trig %d\n", cnt, lsm6dso16is_acc_trig_cnt);
#endif

#ifdef CONFIG_LSM6DSV16X_TRIGGER
		printk("%d: lsm6dsv16x acc trig %d\n", cnt, lsm6dsv16x_acc_trig_cnt);
#endif
#ifdef CONFIG_LPS2XDF_TRIGGER
		printk("%d: lps22df trig %d\n", cnt, lps22df_trig_cnt);
#endif
#ifdef CONFIG_LIS2DUX12_TRIGGER
		printk("%d:: lis2duxs12 acc trig %d\n", cnt, lis2duxs12_acc_trig_cnt);
#endif

		cnt++;
		k_sleep(K_MSEC(2000));
	}
}
