/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>
#include <misc/util.h>
#include <sensor.h>
#include <zephyr.h>
#include <logging/sys_log.h>

QUARK_SE_IPM_DEFINE(ess_ipm, 0, QUARK_SE_IPM_OUTBOUND);

struct channel_info {
	char               *dev_name;
	struct device      *dev;
	int chan;
	struct sensor_value val;
	int id;
};

static struct channel_info info[] = {

#ifdef CONFIG_BME280
	{ "BME280", NULL, SENSOR_CHAN_TEMP, { 0, 0 }, SENSOR_CHAN_TEMP },
	{ "BME280", NULL, SENSOR_CHAN_HUMIDITY, { 0, 0 }, SENSOR_CHAN_HUMIDITY },
	{ "BME280", NULL, SENSOR_CHAN_PRESS, { 0, 0 }, SENSOR_CHAN_PRESS },
#endif

#ifdef CONFIG_ADXL362
	{ "ADXL362", NULL, SENSOR_CHAN_ACCEL_X, { 0, 0 }, SENSOR_CHAN_ACCEL_X },
	{ "ADXL362", NULL, SENSOR_CHAN_ACCEL_Y, { 0, 0 }, SENSOR_CHAN_ACCEL_Y },
	{ "ADXL362", NULL, SENSOR_CHAN_ACCEL_Z, { 0, 0 }, SENSOR_CHAN_ACCEL_Z },
#endif

#ifdef CONFIG_ADXRS290
	{ "ADXRS290", NULL, SENSOR_CHAN_GYRO_X, { 0, 0 }, SENSOR_CHAN_GYRO_X },
	{ "ADXRS290", NULL, SENSOR_CHAN_GYRO_Y, { 0, 0 }, SENSOR_CHAN_GYRO_Y },
#endif

#ifdef CONFIG_SI1153
	{ "SI1153", NULL, SENSOR_CHAN_GESTURE, { 0, 0 }, SENSOR_CHAN_GESTURE },
#endif

};

void main(void)
{
	struct device *ipm;
	struct device *old;
	unsigned int i;
	int rc;
	int ipm_send_complete;

	ipm = device_get_binding("ess_ipm");
	if (ipm == NULL) {
		SYS_LOG_ERR("Failed to get ESS IPM device!");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(info); i++) {
		SYS_LOG_INF("get \"%s\" device", info[i].dev_name);
		info[i].dev = device_get_binding(info[i].dev_name);
		if (info[i].dev == NULL) {
			SYS_LOG_ERR("Failed to get \"%s\" device!", info[i].dev_name);
		}
	}

	while (1) {
		/* fetch sensor samples */
		old = NULL;
		for (i = 0; i < ARRAY_SIZE(info); i++) {
			if (info[i].dev && info[i].dev != old) {
				rc = sensor_sample_fetch(info[i].dev);
				if (rc) {
					SYS_LOG_ERR("Failed to fetch sample for device %s (%d)!",
						    info[i].dev_name, rc);
				}
				old = info[i].dev;
			}
		}

		/* send sensor data to x86 core via IPM */
		ipm_send_complete = 0;
		for (i = 0; i < ARRAY_SIZE(info); i++) {
			if (!info[i].dev) {
				continue;
			}
			/* get sensor channel */
			rc = sensor_channel_get(info[i].dev, info[i].chan, &info[i].val);
			SYS_LOG_INF("Get sample for device %s (%d) %d.%06d!",
				    info[i].dev_name, rc,
				    info[i].val.val1, info[i].val.val2);
			if (rc) {
				SYS_LOG_ERR("Failed to get data for device %s (%d)!",
					    info[i].dev_name, rc);
				continue;
			}

			/* send sensor value to quark via ipm */
			rc = ipm_send(ipm, 1, info[i].id,
				      &info[i].val, sizeof(info[i].val));
			if (rc) {
				SYS_LOG_ERR("Failed to send data for device %s (%d)!",
					    info[i].dev_name, rc);
			}
			ipm_send_complete = 1;
		}

		if (ipm_send_complete) {
			/* send complete to quark via ipm */
			rc = ipm_send(ipm, 1, SENSOR_CHAN_ALL + 1,
				      &info[0].val, sizeof(info[0].val));
			if (rc) {
				SYS_LOG_ERR("Failed to send complete!");
			}
		}

		k_sleep(K_MSEC(300));
	}
}
