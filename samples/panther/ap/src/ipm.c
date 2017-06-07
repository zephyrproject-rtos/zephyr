/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <stdio.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>
#include <sensor.h>
#include <zephyr.h>
#include <misc/printk.h>

#include <mqtt.h>

#include "mqtt.h"

#define STACKSIZE               2000

#define MAIN_FIBER_PRI          2

struct shield_sens {
	struct sensor_value BME280[3];
	struct sensor_value ADXL362[4];
	struct sensor_value ADXRS290[3];
	struct sensor_value SI1153[5];
};

QUARK_SE_IPM_DEFINE(ess_ipm, 0, QUARK_SE_IPM_INBOUND);

static char ipm_stack[STACKSIZE];
static char json[2048];
static struct shield_sens sensors;
static struct k_sem	sync_sem;
static char *mqtt_device_hid;
static char *mqtt_topic;

static void sensor_ipm_callback(void *context, uint32_t id, volatile void *data);
static void ipm_thread(void *arg1, void *arg2, void *arg3);
static int json_get(void);

int ipm_init(char *device_hid, char *topic)
{
	struct device *ipm;
	int ret;

	mqtt_device_hid = device_hid;
	mqtt_topic = topic;

	ipm = device_get_binding("ess_ipm");
	if (!ipm) {
		return -1;
	}

	ipm_register_callback(ipm, sensor_ipm_callback, NULL);

	ret = ipm_set_enabled(ipm, 1);
	if (ret) {
		return -1;
	}

	k_sem_init(&sync_sem, 0, 1);

	k_thread_spawn(ipm_stack, STACKSIZE, ipm_thread,
		       0, 0, 0, K_PRIO_COOP(MAIN_FIBER_PRI), 0, 0);

	return 0;
}

static void sensor_ipm_callback(void *context, uint32_t id,
		volatile void *data)
{
	volatile struct sensor_value *val = data;

	switch (id) {
	case SENSOR_CHAN_TEMP:
		/* resolution of 0.01 degrees Celsius */
		sensors.BME280[0].val1 = val->val1;
		sensors.BME280[0].val2 = val->val2;
		break;
	case SENSOR_CHAN_HUMIDITY:
		/* resolution of 0.01 percent */
		sensors.BME280[1].val1 = val->val1 / 1000;
		sensors.BME280[1].val2 = (val->val1 % 1000)*1000;
		break;
	case SENSOR_CHAN_PRESS:
		/* resolution of 0.1 Pa */
		sensors.BME280[2].val1 = (val->val1 * 10) + (val->val2 / 100000);
		sensors.BME280[2].val2 = (val->val2 % 100000) * 10;
		break;

	case SENSOR_CHAN_ACCEL_Y:
		sensors.ADXL362[0].val1 = -val->val1;
		sensors.ADXL362[0].val2 = -val->val2;
		break;
	case SENSOR_CHAN_ACCEL_X:
		sensors.ADXL362[1].val1 = val->val1;
		sensors.ADXL362[1].val2 = val->val2;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		sensors.ADXL362[2].val1 = -val->val1;
		sensors.ADXL362[2].val2 = -val->val2;

		printk("ipm received, give sem\n");
		k_sem_give(&sync_sem);

		break;

	case SENSOR_CHAN_GYRO_X:
		sensors.ADXRS290[0].val1 = val->val1;
		sensors.ADXRS290[0].val2 = val->val2;
		break;
	case SENSOR_CHAN_GYRO_Y:
		sensors.ADXRS290[1].val1 = val->val1;
		sensors.ADXRS290[1].val2 = val->val2;
		break;

	case SENSOR_CHAN_GESTURE:
		sensors.SI1153[4].val1 = val->val1;
		break;
	default:
		break;
	}
}

static void ipm_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		if (k_sem_take(&sync_sem, K_FOREVER) == 0) {
			if (mqtt_status_get()==1) {
				json_get();
				printk("send mqtt\n%s\n",json);
				mqtt_send(mqtt_topic, json);
			}
		}
	}
}

#define abs(a) ((a)<0?-(a):(a))

int addvalue(char *string, struct sensor_value *value,char *trailer)
{
	int len = 0;

	if (value->val1 <0 || value->val2<0)
	{
		len += sprintf(string+len, "-");
	}
	len += sprintf(string+len, "%d.%06d%s",
			abs(value->val1), abs(value->val2), trailer);

	return len;
}

int json_get(void)
{
	int len = 0;

	len += sprintf(json+len, "{");
	len += sprintf(json+len, "\"_|deviceHid\":\"%s\",", mqtt_device_hid);

	len += sprintf(json+len, "\"f|Temperature\":\"");
	len += addvalue(json+len, &sensors.BME280[0],"\",");

	len += sprintf(json+len, "\"f|Humidity\":\"");
	len += addvalue(json+len, &sensors.BME280[1],"\",");

	len += sprintf(json+len, "\"f|Pressure\":\"");
	len += addvalue(json+len, &sensors.BME280[2],"\",");

	len += sprintf(json+len, "\"f3|AccelXYZ\":\"");
	len += addvalue(json+len, &sensors.ADXL362[0],"|");
	len += addvalue(json+len, &sensors.ADXL362[1],"|");
	len += addvalue(json+len, &sensors.ADXL362[2],"\",");

	len += sprintf(json+len, "\"f|AccelTemp\":\"");
	len += addvalue(json+len, &sensors.ADXL362[3],"\",");

	len += sprintf(json+len, "\"f2|GyroXY\":\"");
	len += addvalue(json+len, &sensors.ADXRS290[0],"|");
	len += addvalue(json+len, &sensors.ADXRS290[1],"\",");

	len += sprintf(json+len, "\"f|GyroTemp\":\"");
	len += addvalue(json+len, &sensors.ADXRS290[2],"\",");

	len += sprintf(json+len, "\"i|Ambient\":\"%d\",",
			sensors.SI1153[0].val1);
	len += sprintf(json+len, "\"i3|Proximity\":\"%d|%d|%d\",",
			sensors.SI1153[1].val1,
			sensors.SI1153[2].val1,
			sensors.SI1153[3].val1);

	len += sprintf(json+len, "\"i|Gesture\":\"%d\"", sensors.SI1153[4].val1);

	len += sprintf(json+len, "}");

	return 0;
}
