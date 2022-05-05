/*
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_LSM6DSL_TRIGGER
static int lsm6dsl_trig_cnt;

static void lsm6dsl_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	lsm6dsl_trig_cnt++;
}
#endif

#define LSM6DSL_DEVNAME		DT_LABEL(DT_INST(0, st_lsm6dsl))

void main(void)
{
#ifdef CONFIG_LSM6DSL_TRIGGER
	int cnt = 1;
#endif
#ifdef CONFIG_LSM6DSL_EXT0_LPS22HB
	struct sensor_value temp, press;
#endif
#ifdef CONFIG_LSM6DSL_EXT0_LIS2MDL
	struct sensor_value magn[3];
#endif
	struct sensor_value accel[3];
	struct sensor_value gyro[3];
	const struct device *lsm6dsl = device_get_binding(LSM6DSL_DEVNAME);

	if (lsm6dsl == NULL) {
		printf("Could not get LSM6DSL device\n");
		return;
	}

	/* set LSM6DSL accel/gyro sampling frequency to 104 Hz */
	struct sensor_value odr_attr;

	odr_attr.val1 = 104;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dsl, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for accelerometer.\n");
		return;
	}

	if (sensor_attr_set(lsm6dsl, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for gyro.\n");
		return;
	}

#ifdef CONFIG_LSM6DSL_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lsm6dsl, &trig, lsm6dsl_trigger_handler);
#endif

	while (1) {
		/* Get sensor samples */

#ifndef CONFIG_LSM6DSL_TRIGGER
		if (sensor_sample_fetch(lsm6dsl) < 0) {
			printf("LSM6DSL Sensor sample update error\n");
			return;
		}
#endif

		/* Get sensor data */

		sensor_channel_get(lsm6dsl, SENSOR_CHAN_ACCEL_XYZ, accel);
		sensor_channel_get(lsm6dsl, SENSOR_CHAN_GYRO_XYZ, gyro);
#ifdef CONFIG_LSM6DSL_EXT0_LPS22HB
		sensor_channel_get(lsm6dsl, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(lsm6dsl, SENSOR_CHAN_PRESS, &press);
#endif
#ifdef CONFIG_LSM6DSL_EXT0_LIS2MDL
		sensor_channel_get(lsm6dsl, SENSOR_CHAN_MAGN_XYZ, magn);
#endif

		/* Display sensor data */

		/* Erase previous */
		printf("\0033\014");

		printf("X-NUCLEO-IKS01A2 sensor dashboard\n\n");

		/* lsm6dsl accel */
		printf("LSM6DSL: Accel (m.s-2): x: %.1f, y: %.1f, z: %.1f\n",
		       sensor_value_to_double(&accel[0]),
		       sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]));

		/* lsm6dsl gyro */
		printf("LSM6DSL: Gyro (dps): x: %.3f, y: %.3f, z: %.3f\n",
		       sensor_value_to_double(&gyro[0]),
		       sensor_value_to_double(&gyro[1]),
		       sensor_value_to_double(&gyro[2]));

#ifdef CONFIG_LSM6DSL_EXT0_LPS22HB
		printf("LSM6DSL: Temperature: %.1f C\n",
		       sensor_value_to_double(&temp));

		printf("LSM6DSL: Pressure:%.3f kpa\n",
		       sensor_value_to_double(&press));
#endif

#ifdef CONFIG_LSM6DSL_EXT0_LIS2MDL
		printf("LSM6DSL: Magn (gauss): x: %.3f, y: %.3f, z: %.3f\n",
		       sensor_value_to_double(&magn[0]),
		       sensor_value_to_double(&magn[1]),
		       sensor_value_to_double(&magn[2]));
#endif

#ifdef CONFIG_LSM6DSL_TRIGGER
		printk("%d:: lsm6dsl acc trig %d\n", cnt++, lsm6dsl_trig_cnt);
#endif

		k_sleep(K_MSEC(2000));
	}
}
