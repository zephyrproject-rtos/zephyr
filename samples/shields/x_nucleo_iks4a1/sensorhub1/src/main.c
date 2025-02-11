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

#ifdef CONFIG_LSM6DSV16X_TRIGGER
static int lsm6dsv16x_acc_trig_cnt;

static void lsm6dsv16x_acc_trig_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	lsm6dsv16x_acc_trig_cnt++;
}
#endif

static void lsm6dsv16x_config(const struct device *lsm6dsv16x)
{
	struct sensor_value odr_attr, fs_attr;

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

	/* set LSM6DSV16X external magn sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

#ifdef CONFIG_LSM6DSV16X_EXT_LIS2MDL
	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_MAGN_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSV16X ext magn\n");
	}
#endif

#ifdef CONFIG_LSM6DSV16X_EXT_LPS22DF
	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_PRESS,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSV16X ext pressure\n");
	}
#endif

#ifdef CONFIG_LSM6DSV16X_EXT_HTS221
	odr_attr.val1 = 12;
	if (sensor_attr_set(lsm6dsv16x, SENSOR_CHAN_HUMIDITY,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSV16X ext humidity\n");
	}
#endif

#ifdef CONFIG_LSM6DSV16X_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lsm6dsv16x, &trig, lsm6dsv16x_acc_trig_handler);
#endif
}

int main(void)
{
	struct sensor_value lsm6dsv16x_xl[3], lsm6dsv16x_gy[3];
#ifdef CONFIG_LSM6DSV16X_ENABLE_TEMP
	struct sensor_value lsm6dsv16x_temp;
#endif
#ifdef CONFIG_LSM6DSV16X_EXT_LIS2MDL
	struct sensor_value lis2mdl_magn[3];
#endif
#ifdef CONFIG_LSM6DSV16X_EXT_LPS22DF
	struct sensor_value lps22df_press;
	struct sensor_value lps22df_temp;
#endif
	const struct device *const lsm6dsv16x = DEVICE_DT_GET_ONE(st_lsm6dsv16x);
	int cnt = 1;

	if (!device_is_ready(lsm6dsv16x)) {
		printk("%s: device not ready.\n", lsm6dsv16x->name);
		return 0;
	}

	lsm6dsv16x_config(lsm6dsv16x);

	while (1) {
		/* Get sensor samples */
#ifndef CONFIG_LSM6DSV16X_TRIGGER
		if (sensor_sample_fetch(lsm6dsv16x) < 0) {
			printf("LSM6DSV16X Sensor sample update error\n");
			return 0;
		}
#endif

		/* Get sensor data */
		sensor_channel_get(lsm6dsv16x, SENSOR_CHAN_ACCEL_XYZ, lsm6dsv16x_xl);
		sensor_channel_get(lsm6dsv16x, SENSOR_CHAN_GYRO_XYZ, lsm6dsv16x_gy);
#ifdef CONFIG_LSM6DSV16X_ENABLE_TEMP
		sensor_channel_get(lsm6dsv16x, SENSOR_CHAN_DIE_TEMP, &lsm6dsv16x_temp);
#endif
#ifdef CONFIG_LSM6DSV16X_EXT_LIS2MDL
		sensor_channel_get(lsm6dsv16x, SENSOR_CHAN_MAGN_XYZ, lis2mdl_magn);
#endif
#ifdef CONFIG_LSM6DSV16X_EXT_LPS22DF
		sensor_channel_get(lsm6dsv16x, SENSOR_CHAN_AMBIENT_TEMP, &lps22df_temp);
		sensor_channel_get(lsm6dsv16x, SENSOR_CHAN_PRESS, &lps22df_press);
#endif

		/* Display sensor data */

		/* Erase previous */
		printf("\0033\014");

		printf("X-NUCLEO-IKS01A4 sensor dashboard\n\n");

		printf("LSM6DSV16X: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lsm6dsv16x_xl[0]),
			sensor_value_to_double(&lsm6dsv16x_xl[1]),
			sensor_value_to_double(&lsm6dsv16x_xl[2]));

		printf("LSM6DSV16X: Gyro (dps): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&lsm6dsv16x_gy[0]),
			sensor_value_to_double(&lsm6dsv16x_gy[1]),
			sensor_value_to_double(&lsm6dsv16x_gy[2]));

#ifdef CONFIG_LSM6DSV16X_ENABLE_TEMP
		/* temperature */
		printf("LSM6DSV16X: Temperature: %.1f C\n",
		       sensor_value_to_double(&lsm6dsv16x_temp));
#endif

#ifdef CONFIG_LSM6DSV16X_EXT_LIS2MDL
		printf("LSM6DSV16X: Magn (gauss): x: %.3f, y: %.3f, z: %.3f\n",
		       sensor_value_to_double(&lis2mdl_magn[0]),
		       sensor_value_to_double(&lis2mdl_magn[1]),
		       sensor_value_to_double(&lis2mdl_magn[2]));
#endif

#ifdef CONFIG_LSM6DSV16X_EXT_LPS22DF
		printf("LSM6DSV16X: Temperature: %.1f C\n",
		       sensor_value_to_double(&lps22df_temp));

		printf("LSM6DSV16X: Pressure:%.3f kpa\n",
		       sensor_value_to_double(&lps22df_press));
#endif

#ifdef CONFIG_LSM6DSV16X_TRIGGER
		printk("%d: lsm6dsv16x acc trig %d\n", cnt, lsm6dsv16x_acc_trig_cnt);
#endif

		cnt++;
		k_sleep(K_MSEC(2000));
	}
}
