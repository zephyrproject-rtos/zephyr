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

#ifdef CONFIG_LSM6DSO16IS_TRIGGER
static int lsm6dso16is_acc_trig_cnt;

static void lsm6dso16is_acc_trig_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	lsm6dso16is_acc_trig_cnt++;
}
#endif

static void lsm6dso16is_config(const struct device *lsm6dso16is)
{
	struct sensor_value odr_attr, fs_attr;

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

	/* set LSM6DSO16IS external magn sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

#ifdef CONFIG_LSM6DSO16IS_EXT_LIS2MDL
	if (sensor_attr_set(lsm6dso16is, SENSOR_CHAN_MAGN_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO16IS ext magn\n");
	}
#endif

#ifdef CONFIG_LSM6DSO16IS_EXT_LPS22DF
	if (sensor_attr_set(lsm6dso16is, SENSOR_CHAN_PRESS,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO16IS ext pressure\n");
	}
#endif

#ifdef CONFIG_LSM6DSO16IS_EXT_HTS221
	odr_attr.val1 = 12;
	if (sensor_attr_set(lsm6dso16is, SENSOR_CHAN_HUMIDITY,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO16IS ext humidity\n");
	}
#endif

#ifdef CONFIG_LSM6DSO16IS_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lsm6dso16is, &trig, lsm6dso16is_acc_trig_handler);
#endif
}

int main(void)
{
	struct sensor_value lsm6dso16is_xl[3], lsm6dso16is_gy[3];
#ifdef CONFIG_LSM6DSO16IS_ENABLE_TEMP
	struct sensor_value lsm6dso16is_temp;
#endif
#ifdef CONFIG_LSM6DSO16IS_EXT_LIS2MDL
	struct sensor_value lis2mdl_magn[3];
#endif
#ifdef CONFIG_LSM6DSO16IS_EXT_LPS22DF
	struct sensor_value lps22df_press;
	struct sensor_value lps22df_temp;
#endif
	const struct device *const lsm6dso16is = DEVICE_DT_GET_ONE(st_lsm6dso16is);
	int cnt = 1;

	if (!device_is_ready(lsm6dso16is)) {
		printk("%s: device not ready.\n", lsm6dso16is->name);
		return 0;
	}

	lsm6dso16is_config(lsm6dso16is);

	while (1) {
		/* Get sensor samples */
#ifndef CONFIG_LSM6DSO16IS_TRIGGER
		if (sensor_sample_fetch(lsm6dso16is) < 0) {
			printf("LSM6DSO16IS Sensor sample update error\n");
			return 0;
		}
#endif

		/* Get sensor data */
		sensor_channel_get(lsm6dso16is, SENSOR_CHAN_ACCEL_XYZ, lsm6dso16is_xl);
		sensor_channel_get(lsm6dso16is, SENSOR_CHAN_GYRO_XYZ, lsm6dso16is_gy);
#ifdef CONFIG_LSM6DSO16IS_ENABLE_TEMP
		sensor_channel_get(lsm6dso16is, SENSOR_CHAN_DIE_TEMP, &lsm6dso16is_temp);
#endif
#ifdef CONFIG_LSM6DSO16IS_EXT_LIS2MDL
		sensor_channel_get(lsm6dso16is, SENSOR_CHAN_MAGN_XYZ, lis2mdl_magn);
#endif
#ifdef CONFIG_LSM6DSO16IS_EXT_LPS22DF
		sensor_channel_get(lsm6dso16is, SENSOR_CHAN_AMBIENT_TEMP, &lps22df_temp);
		sensor_channel_get(lsm6dso16is, SENSOR_CHAN_PRESS, &lps22df_press);
#endif

		/* Display sensor data */

		/* Erase previous */
		printf("\0033\014");

		printf("X-NUCLEO-IKS01A4 sensor dashboard\n\n");

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

#ifdef CONFIG_LSM6DSO16IS_EXT_LIS2MDL
		printf("LSM6DSO16IS: Magn (gauss): x: %.3f, y: %.3f, z: %.3f\n",
		       sensor_value_to_double(&lis2mdl_magn[0]),
		       sensor_value_to_double(&lis2mdl_magn[1]),
		       sensor_value_to_double(&lis2mdl_magn[2]));
#endif

#ifdef CONFIG_LSM6DSO16IS_EXT_LPS22DF
		printf("LSM6DSO16IS: Temperature: %.1f C\n",
		       sensor_value_to_double(&lps22df_temp));

		printf("LSM6DSO16IS: Pressure:%.3f kpa\n",
		       sensor_value_to_double(&lps22df_press));
#endif

#ifdef CONFIG_LSM6DSO16IS_TRIGGER
		printk("%d: lsm6dso16is acc trig %d\n", cnt, lsm6dso16is_acc_trig_cnt);
#endif

		cnt++;
		k_sleep(K_MSEC(2000));
	}
}
