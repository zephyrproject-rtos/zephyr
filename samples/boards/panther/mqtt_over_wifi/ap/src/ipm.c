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
#include <json.h>
#include <mqtt.h>
#include <misc/printk.h>
#include <logging/sys_log.h>

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
static struct shield_sens sensors;
static struct k_sem sync_sem;
static char *mqtt_topic;
struct k_thread ipm_thread_data;

static void sensor_ipm_callback(void *context, u32_t id, volatile void *data);
static void ipm_thread(void *arg1, void *arg2, void *arg3);
static char *json_get(void);

int ipm_init(char *topic)
{
	struct device *ipm;
	int ret;

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

	k_thread_create(&ipm_thread_data, ipm_stack, STACKSIZE, ipm_thread,
		       0, 0, 0, K_PRIO_COOP(MAIN_FIBER_PRI), 0, 0);

	return 0;
}

static void sensor_ipm_callback(void *context, u32_t id,
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
		sensors.BME280[1].val2 = (val->val1 % 1000) * 1000;
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
	case SENSOR_CHAN_ALL + 1:
		SYS_LOG_INF("ipm received, give sem");
		k_sem_give(&sync_sem);
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
			if (mqtt_status_get() == 1) {
				char *json;

				json = json_get();
				if (json) {
					SYS_LOG_INF("JSON TO SEND:\n%s\n", json);
					mqtt_send(mqtt_topic, json);
				}
			}
		}
	}
}

#define abs(a) ((a) < 0 ? -(a) : (a))

#define FIELD(struct_, member_, type_) {		\
		.field_name = #member_,			\
		.field_name_len = sizeof(#member_) - 1,	\
		.offset = offsetof(struct_, member_),	\
		.type = type_				\
}

#define STRING_VALUE_SIZE 16

struct json_struct {
	char *tmp;
	char *hum;
	char *atm;
	char *acx;
	char *acy;
	char *acz;
	char *gyx;
	char *gyy;
	int gst;
};

static const struct json_obj_descr json_descr[] = {
	FIELD(struct json_struct, tmp, JSON_TOK_STRING),
	FIELD(struct json_struct, hum, JSON_TOK_STRING),
	FIELD(struct json_struct, atm, JSON_TOK_STRING),
	FIELD(struct json_struct, acx, JSON_TOK_STRING),
	FIELD(struct json_struct, acy, JSON_TOK_STRING),
	FIELD(struct json_struct, acz, JSON_TOK_STRING),
	FIELD(struct json_struct, gyx, JSON_TOK_STRING),
	FIELD(struct json_struct, gyy, JSON_TOK_STRING),
	FIELD(struct json_struct, gst, JSON_TOK_NUMBER),
};

int value_get(char *string, struct sensor_value *value)
{
	int len = 0;

	if (value->val1 < 0 || value->val2 < 0) {
		len += snprintk(string + len, STRING_VALUE_SIZE, "-");
	}
	len += snprintk(string + len, STRING_VALUE_SIZE, "%d.%06d",
			abs(value->val1), abs(value->val2));

	return len;
}

char *json_get(void)
{
	static char json_buffer[1024];
	char sensor_value[8][STRING_VALUE_SIZE];
	struct json_struct js;
	int ret;

	value_get(sensor_value[0], &sensors.BME280[0]);
	value_get(sensor_value[1], &sensors.BME280[1]);
	value_get(sensor_value[2], &sensors.BME280[2]);
	value_get(sensor_value[3], &sensors.ADXL362[0]);
	value_get(sensor_value[4], &sensors.ADXL362[1]);
	value_get(sensor_value[5], &sensors.ADXL362[2]);
	value_get(sensor_value[6], &sensors.ADXRS290[0]);
	value_get(sensor_value[7], &sensors.ADXRS290[1]);

	js.tmp = sensor_value[0];
	js.hum = sensor_value[1];
	js.atm = sensor_value[2];
	js.acx = sensor_value[3];
	js.acy = sensor_value[4];
	js.acz = sensor_value[5];
	js.gyx = sensor_value[6];
	js.gyy = sensor_value[7];
	js.gst = sensors.SI1153[4].val1;

	ret = json_obj_encode_buf(json_descr, ARRAY_SIZE(json_descr),
				  &js, json_buffer, sizeof(json_buffer));
	if (ret) {
		SYS_LOG_ERR("json_obj_encode_buf FAIL!");
		return NULL;
	}
	return json_buffer;
}
