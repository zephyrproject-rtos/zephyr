/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <sensor.h>
#include <zephyr.h>

//#define DEBUG

#ifdef DEBUG
#define DBG_PRINT(...) printk(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

QUARK_SE_IPM_DEFINE(ess_ipm, 0, QUARK_SE_IPM_OUTBOUND);

struct channel_info {
	char               *dev_name;
	struct device      *dev;
	int                 chan;
	struct sensor_value val;
	int                 id;
};

static struct channel_info info[] = {
#ifdef CONFIG_BME280
	{"BME280", NULL, SENSOR_CHAN_TEMP    , {0, 0}, SENSOR_CHAN_TEMP},//0x00},
	{"BME280", NULL, SENSOR_CHAN_HUMIDITY, {0, 0}, SENSOR_CHAN_HUMIDITY},//0x01},
	{"BME280", NULL, SENSOR_CHAN_PRESS   , {0, 0}, SENSOR_CHAN_PRESS},//0x02},
#endif

#ifdef CONFIG_ADXL362
	{"ADXL362", NULL, SENSOR_CHAN_ACCEL_X, {0, 0}, SENSOR_CHAN_ACCEL_X},//0x10},
	{"ADXL362", NULL, SENSOR_CHAN_ACCEL_Y, {0, 0}, SENSOR_CHAN_ACCEL_Y},//0x11},
	{"ADXL362", NULL, SENSOR_CHAN_ACCEL_Z, {0, 0}, SENSOR_CHAN_ACCEL_Z},//0x12},
//	{"ADXL362", NULL, SENSOR_CHAN_TEMP   , {0, 0}, 0x13},
#endif

#ifdef CONFIG_ADXRS290
	{"ADXRS290", NULL, SENSOR_CHAN_GYRO_X, {0, 0}, SENSOR_CHAN_GYRO_X},//0x20},
	{"ADXRS290", NULL, SENSOR_CHAN_GYRO_Y, {0, 0}, SENSOR_CHAN_GYRO_Y},//0x21},
//	{"ADXRS290", NULL, SENSOR_CHAN_TEMP  , {0, 0}, 0x22},
#endif

#ifdef CONFIG_SI1153
	{"SI1153", NULL, SENSOR_CHAN_GESTURE, {0, 0}, 0x30},
#endif
};

void main(void)
{
	struct device *ipm;
	struct device *old;
	unsigned int i;
	int rc;

	ipm = device_get_binding("ess_ipm");
	if (ipm == NULL) {
		printk("Failed to get ESS IPM device\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(info); i++) {
		DBG_PRINT("get \"%s\" device\n", info[i].dev_name);
		info[i].dev = device_get_binding(info[i].dev_name);
		if (info[i].dev == NULL) {
			printk("Failed to get \"%s\" device\n", info[i].dev_name);
			//return;
		}
	}

	while (1) {

		/* fetch sensor samples */
		old = NULL;
		for (i = 0; i < ARRAY_SIZE(info); i++) {
			if (info[i].dev) {
				if(info[i].dev != old){
					rc = sensor_sample_fetch(info[i].dev);
					if (rc) {
						printk("Failed to fetch sample for device %s (%d)\n",
							   info[i].dev_name, rc);
					}
					old = info[i].dev;
				}
			}
		}

		/* send sensor data to x86 core via IPM */
		for (i = 0; i < ARRAY_SIZE(info); i++) {
			if (info[i].dev) {
				/* get sensor channel */
				rc = sensor_channel_get(info[i].dev, info[i].chan, &info[i].val);
				DBG_PRINT("Get sample for device %s (%d) %d.%06d\n",
					   info[i].dev_name, rc,
					   info[i].val.val1, info[i].val.val2);
				if (rc) {
					printk("Failed to get data for device %s (%d)\n",
						   info[i].dev_name, rc);
					continue;
				}

				/* send sensor value to quark via ipm */
				rc = ipm_send(ipm, 1, info[i].id,
						&info[i].val, sizeof(info[i].val));
				if (rc) {
					printk("Failed to send data for device %s (%d)\n",
						   info[i].dev_name, rc);
				}

			}
		}
		k_sleep(K_MSEC(300));
	}
}
