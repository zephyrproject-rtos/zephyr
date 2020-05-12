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

#ifdef CONFIG_IIS2DLPC_TRIGGER
static int iis2dlpc_trig_cnt;

static void iis2dlpc_trigger_handler(const struct device *dev,
				     struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	iis2dlpc_trig_cnt++;
}
#endif

#ifdef CONFIG_ISM330DHCX_TRIGGER
static int ism330dhcx_acc_trig_cnt;
static int ism330dhcx_gyr_trig_cnt;
static int ism330dhcx_temp_trig_cnt;

static void ism330dhcx_acc_trig_handler(const struct device *dev,
					struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	ism330dhcx_acc_trig_cnt++;
}

static void ism330dhcx_gyr_trig_handler(const struct device *dev,
					struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	ism330dhcx_gyr_trig_cnt++;
}

static void ism330dhcx_temp_trig_handler(const struct device *dev,
					 struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_DIE_TEMP);
	ism330dhcx_temp_trig_cnt++;
}
#endif

static void iis2dlpc_config(const struct device *iis2dlpc)
{
	struct sensor_value odr_attr, fs_attr;

	/* set IIS2DLPC accel/gyro sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

	if (sensor_attr_set(iis2dlpc, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for IIS2DLPC accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(iis2dlpc, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set sampling frequency for IIS2DLPC gyro\n");
		return;
	}

#ifdef CONFIG_IIS2DLPC_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(iis2dlpc, &trig, iis2dlpc_trigger_handler);
#endif
}

static void ism330dhcx_config(const struct device *ism330dhcx)
{
	struct sensor_value odr_attr, fs_attr;

	/* set ISM330DHCX accel sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for ISM330DHCX accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for ISM330DHCX accel\n");
		return;
	}

	/* set ISM330DHCX gyro sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for ISM330DHCX gyro\n");
		return;
	}

	sensor_degrees_to_rad(250, &fs_attr);

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for ISM330DHCX gyro\n");
		return;
	}

	/* set ISM330DHCX external magn sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

#ifdef CONFIG_ISM330DHCX_EXT_IIS2MDC
	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_MAGN_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for ISM330DHCX ext magn\n");
		return;
	}
#endif

#ifdef CONFIG_ISM330DHCX_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(ism330dhcx, &trig, ism330dhcx_acc_trig_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;
	sensor_trigger_set(ism330dhcx, &trig, ism330dhcx_gyr_trig_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_DIE_TEMP;
	sensor_trigger_set(ism330dhcx, &trig, ism330dhcx_temp_trig_handler);
#endif
}

void main(void)
{
#ifdef CONFIG_ISM330DHCX_ENABLE_TEMP
	struct sensor_value die_temp;
#endif
	struct sensor_value accel1[3], accel2[3];
	struct sensor_value gyro[3];
	struct sensor_value magn[3];
	const struct device *iis2dlpc = device_get_binding(DT_LABEL(DT_INST(0, st_iis2dlpc)));
	const struct device *ism330dhcx = device_get_binding(DT_LABEL(DT_INST(0, st_ism330dhcx)));
	int cnt = 1;

	if (iis2dlpc == NULL) {
		printf("Could not get IIS2DLPC device\n");
		return;
	}
	if (ism330dhcx == NULL) {
		printf("Could not get ISM330DHCX device\n");
		return;
	}

	iis2dlpc_config(iis2dlpc);
	ism330dhcx_config(ism330dhcx);

	while (1) {
		/* Get sensor samples */

#ifndef CONFIG_IIS2DLPC_TRIGGER
		if (sensor_sample_fetch(iis2dlpc) < 0) {
			printf("IIS2DLPC Sensor sample update error\n");
			return;
		}
#endif
#ifndef CONFIG_ISM330DHCX_TRIGGER
		if (sensor_sample_fetch(ism330dhcx) < 0) {
			printf("ISM330DHCX Sensor sample update error\n");
			return;
		}
#endif

		/* Get sensor data */
		sensor_channel_get(iis2dlpc, SENSOR_CHAN_ACCEL_XYZ, accel2);
		sensor_channel_get(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ, accel1);
		sensor_channel_get(ism330dhcx, SENSOR_CHAN_GYRO_XYZ, gyro);
#ifdef CONFIG_ISM330DHCX_ENABLE_TEMP
		sensor_channel_get(ism330dhcx, SENSOR_CHAN_DIE_TEMP, &die_temp);
#endif
#ifdef CONFIG_ISM330DHCX_EXT_IIS2MDC
		sensor_channel_get(ism330dhcx, SENSOR_CHAN_MAGN_XYZ, magn);
#endif

		/* Display sensor data */

		/* Erase previous */
		printf("\0033\014");

		printf("X-NUCLEO-IKS02A1 sensor Mode 2 dashboard\n\n");

		printf("IIS2DLPC: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&accel2[0]),
			sensor_value_to_double(&accel2[1]),
			sensor_value_to_double(&accel2[2]));

		printf("ISM330DHCX: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&accel1[0]),
			sensor_value_to_double(&accel1[1]),
			sensor_value_to_double(&accel1[2]));

		printf("ISM330DHCX: GYro (dps): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&gyro[0]),
			sensor_value_to_double(&gyro[1]),
			sensor_value_to_double(&gyro[2]));

#ifdef CONFIG_ISM330DHCX_ENABLE_TEMP
		/* temperature */
		printf("ISM330DHCX: Temperature: %.1f C\n",
		       sensor_value_to_double(&die_temp));
#endif

#ifdef CONFIG_ISM330DHCX_EXT_IIS2MDC
		printf("ISM330DHCX: Magn (gauss): x: %.3f, y: %.3f, z: %.3f\n",
		       sensor_value_to_double(&magn[0]),
		       sensor_value_to_double(&magn[1]),
		       sensor_value_to_double(&magn[2]));
#endif

#ifdef CONFIG_IIS2DLPC_TRIGGER
		printk("%d:: iis2dlpc trig %d\n", cnt, iis2dlpc_trig_cnt);
#endif

#ifdef CONFIG_ISM330DHCX_TRIGGER
		printk("%d:: ism330dhcx acc trig %d\n", cnt, ism330dhcx_acc_trig_cnt);
		printk("%d:: ism330dhcx gyr trig %d\n", cnt, ism330dhcx_gyr_trig_cnt);
#ifdef CONFIG_ISM330DHCX_ENABLE_TEMP
		printk("%d:: ism330dhcx temp trig %d\n", cnt, ism330dhcx_temp_trig_cnt);
#endif
#endif

		cnt++;
		k_sleep(K_MSEC(2000));
	}
}
