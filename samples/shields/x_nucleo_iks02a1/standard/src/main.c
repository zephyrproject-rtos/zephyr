/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_IIS2DLPC_TRIGGER
static int iis2dlpc_trig_cnt;

static void iis2dlpc_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	iis2dlpc_trig_cnt++;
}
#endif

#ifdef CONFIG_IIS2MDC_TRIGGER
static int iis2mdc_trig_cnt;

static void iis2mdc_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
	iis2mdc_trig_cnt++;
}
#endif

#ifdef CONFIG_ISM330DHCX_TRIGGER
static int ism330dhcx_acc_trig_cnt;
static int ism330dhcx_gyr_trig_cnt;

static void ism330dhcx_acc_trigger_handler(const struct device *dev,
					   const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	ism330dhcx_acc_trig_cnt++;
}

static void ism330dhcx_gyr_trigger_handler(const struct device *dev,
					   const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	ism330dhcx_gyr_trig_cnt++;
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

static void iis2mdc_config(const struct device *iis2mdc)
{
	struct sensor_value odr_attr;

	/* set IIS2MDC sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

	if (sensor_attr_set(iis2mdc, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for IIS2MDC\n");
		return;
	}

#ifdef CONFIG_IIS2MDC_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_MAGN_XYZ;
	sensor_trigger_set(iis2mdc, &trig, iis2mdc_trigger_handler);
#endif
}

static void ism330dhcx_config(const struct device *ism330dhcx)
{
	struct sensor_value odr_attr, fs_attr;

	/* set ISM330DHCX sampling frequency to 416 Hz */
	odr_attr.val1 = 416;
	odr_attr.val2 = 0;

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for ISM330DHCX accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set sampling frequency for ISM330DHCX accel\n");
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

#ifdef CONFIG_ISM330DHCX_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(ism330dhcx, &trig, ism330dhcx_acc_trigger_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;
	sensor_trigger_set(ism330dhcx, &trig, ism330dhcx_gyr_trigger_handler);
#endif
}

void main(void)
{
#ifdef CONFIG_ISM330DHCX_ENABLE_TEMP
	struct sensor_value die_temp;
#endif
	struct sensor_value die_temp2;
	struct sensor_value accel1[3], accel2[3];
	struct sensor_value gyro[3];
	struct sensor_value magn[3];
	const struct device *iis2dlpc = DEVICE_DT_GET_ONE(st_iis2dlpc);
	const struct device *iis2mdc = DEVICE_DT_GET_ONE(st_iis2mdc);
	const struct device *ism330dhcx = DEVICE_DT_GET_ONE(st_ism330dhcx);
	int cnt = 1;

	if (!device_is_ready(iis2dlpc)) {
		printk("%s: device not ready.\n", iis2dlpc->name);
		return;
	}
	if (!device_is_ready(iis2mdc)) {
		printk("%s: device not ready.\n", iis2mdc->name);
		return;
	}
	if (!device_is_ready(ism330dhcx)) {
		printk("%s: device not ready.\n", ism330dhcx->name);
		return;
	}

	iis2dlpc_config(iis2dlpc);
	iis2mdc_config(iis2mdc);
	ism330dhcx_config(ism330dhcx);

	while (1) {
		/* Get sensor samples */

#ifndef CONFIG_IIS2DLPC_TRIGGER
		if (sensor_sample_fetch(iis2dlpc) < 0) {
			printf("IIS2DLPC Sensor sample update error\n");
			return;
		}
#endif
#ifndef CONFIG_IIS2MDC_TRIGGER
		if (sensor_sample_fetch(iis2mdc) < 0) {
			printf("IIS2MDC Magn Sensor sample update error\n");
			return;
		}
#endif
#ifndef CONFIG_ISM330DHCX_TRIGGER
		if (sensor_sample_fetch(ism330dhcx) < 0) {
			printf("ISM330DHCX IMU Sensor sample update error\n");
			return;
		}
#endif

		/* Get sensor data */

		sensor_channel_get(iis2dlpc, SENSOR_CHAN_ACCEL_XYZ, accel2);
		sensor_channel_get(iis2mdc, SENSOR_CHAN_MAGN_XYZ, magn);
		sensor_channel_get(iis2mdc, SENSOR_CHAN_DIE_TEMP, &die_temp2);
		sensor_channel_get(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ, accel1);
		sensor_channel_get(ism330dhcx, SENSOR_CHAN_GYRO_XYZ, gyro);

		/* Display sensor data */

		/* Erase previous */
		printf("\0033\014");

		printf("X-NUCLEO-IKS02A1 sensor Mode 1 dashboard\n\n");

		printf("IIS2DLPC: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&accel2[0]),
			sensor_value_to_double(&accel2[1]),
			sensor_value_to_double(&accel2[2]));

		/* iis2mdc */
		printf("IIS2MDC: Magn (gauss): x: %.3f, y: %.3f, z: %.3f\n",
		       sensor_value_to_double(&magn[0]),
		       sensor_value_to_double(&magn[1]),
		       sensor_value_to_double(&magn[2]));

		printf("IIS2MDC: Temperature: %.1f C\n",
		       sensor_value_to_double(&die_temp2));

		printf("ISM330DHCX: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&accel1[0]),
			sensor_value_to_double(&accel1[1]),
			sensor_value_to_double(&accel1[2]));

		printf("ISM330DHCX: GYro (dps): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&gyro[0]),
			sensor_value_to_double(&gyro[1]),
			sensor_value_to_double(&gyro[2]));
#ifdef CONFIG_IIS2DLPC_TRIGGER
		printk("%d:: iis2dlpc trig %d\n", cnt, iis2dlpc_trig_cnt);
#endif

#if defined(CONFIG_IIS2MDC_TRIGGER)
		printk("%d:: iis2mdc trig %d\n", cnt, iis2mdc_trig_cnt);
#endif

#ifdef CONFIG_ISM330DHCX_TRIGGER
		printk("%d:: ism330dhcx acc trig %d\n", cnt, ism330dhcx_acc_trig_cnt);
		printk("%d:: ism330dhcx gyr trig %d\n", cnt, ism330dhcx_gyr_trig_cnt);
#endif


		cnt++;
		k_sleep(K_MSEC(2000));
	}
}
