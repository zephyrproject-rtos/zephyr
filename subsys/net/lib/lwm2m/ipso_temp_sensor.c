/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for IPSO Temperature Sensor object (3303):
 * https://github.com/IPSO-Alliance/pub/blob/master/docs/IPSO-Smart-Objects.pdf
 * Section: "10. IPSO Object: Temperature"
 */

#define SYS_LOG_DOMAIN "ipso_temp_sensor"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>
#include <stdint.h>
#include <init.h>
#include <net/lwm2m.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

/* Server resource IDs */
#define TEMP_SENSOR_VALUE_ID			5700
#define TEMP_UNITS_ID				5701
#define TEMP_MIN_MEASURED_VALUE_ID		5601
#define TEMP_MAX_MEASURED_VALUE_ID		5602
#define TEMP_MIN_RANGE_VALUE_ID			5603
#define TEMP_MAX_RANGE_VALUE_ID			5604
#define TEMP_RESET_MIN_MAX_MEASURED_VALUES_ID	5605

#define TEMP_MAX_ID		7

#define MAX_INSTANCE_COUNT	CONFIG_LWM2M_IPSO_TEMP_SENSOR_INSTANCE_COUNT

#define TEMP_STRING_SHORT	8

/* resource state variables */
static float32_value_t sensor_value[MAX_INSTANCE_COUNT];
static char units[MAX_INSTANCE_COUNT][TEMP_STRING_SHORT];
static float32_value_t min_measured_value[MAX_INSTANCE_COUNT];
static float32_value_t max_measured_value[MAX_INSTANCE_COUNT];
static float32_value_t min_range_value[MAX_INSTANCE_COUNT];
static float32_value_t max_range_value[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj temp_sensor;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(TEMP_SENSOR_VALUE_ID, R, FLOAT32),
	OBJ_FIELD_DATA(TEMP_UNITS_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(TEMP_MIN_MEASURED_VALUE_ID, R_OPT, FLOAT32),
	OBJ_FIELD_DATA(TEMP_MAX_MEASURED_VALUE_ID, R_OPT, FLOAT32),
	OBJ_FIELD_DATA(TEMP_MIN_RANGE_VALUE_ID, R_OPT, FLOAT32),
	OBJ_FIELD_DATA(TEMP_MAX_RANGE_VALUE_ID, R_OPT, FLOAT32),
	OBJ_FIELD_EXECUTE_OPT(TEMP_RESET_MIN_MAX_MEASURED_VALUES_ID),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res_inst res[MAX_INSTANCE_COUNT][TEMP_MAX_ID];

static void update_min_measured(u16_t obj_inst_id, int index)
{
	min_measured_value[index].val1 = sensor_value[index].val1;
	min_measured_value[index].val2 = sensor_value[index].val2;
	NOTIFY_OBSERVER(IPSO_OBJECT_TEMP_SENSOR_ID, obj_inst_id,
			TEMP_MIN_MEASURED_VALUE_ID);
}

static void update_max_measured(u16_t obj_inst_id, int index)
{
	max_measured_value[index].val1 = sensor_value[index].val1;
	max_measured_value[index].val2 = sensor_value[index].val2;
	NOTIFY_OBSERVER(IPSO_OBJECT_TEMP_SENSOR_ID, obj_inst_id,
			TEMP_MAX_MEASURED_VALUE_ID);
}

static int reset_min_max_measured_values_cb(u16_t obj_inst_id)
{
	int i;

	SYS_LOG_DBG("RESET MIN/MAX %d", obj_inst_id);
	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			update_min_measured(obj_inst_id, i);
			update_max_measured(obj_inst_id, i);
			return 0;
		}
	}

	return -ENOENT;
}

static int sensor_value_write_cb(u16_t obj_inst_id,
				 u8_t *data, u16_t data_len,
				 bool last_block, size_t total_size)
{
	int i;
	bool update_min = false;
	bool update_max = false;

	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			/* update min / max */
			if (sensor_value[i].val1 < min_measured_value[i].val1) {
				update_min = true;
			} else if (sensor_value[i].val1 ==
					min_measured_value[i].val1 &&
				   sensor_value[i].val2 <
					min_measured_value[i].val2) {
				update_min = true;
			}

			if (sensor_value[i].val1 > max_measured_value[i].val1) {
				update_max = true;
			} else if (sensor_value[i].val1 ==
					max_measured_value[i].val1 &&
				   sensor_value[i].val2 >
					max_measured_value[i].val2) {
				update_max = true;
			}

			if (update_min) {
				update_min_measured(obj_inst_id, i);
			}

			if (update_max) {
				update_max_measured(obj_inst_id, i);
			}
		}
	}

	return 0;
}

static struct lwm2m_engine_obj_inst *temp_sensor_create(u16_t obj_inst_id)
{
	int index, i = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			SYS_LOG_ERR("Can not create instance - "
				    "already existing: %u", obj_inst_id);
			return NULL;
		}
	}

	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (!inst[index].obj) {
			break;
		}
	}

	if (index >= MAX_INSTANCE_COUNT) {
		SYS_LOG_ERR("Can not create instance - "
			    "no more room: %u", obj_inst_id);
		return NULL;
	}

	/* Set default values */
	sensor_value[index].val1 = 0;
	sensor_value[index].val2 = 0;
	units[index][0] = '\0';
	min_measured_value[index].val1 = INT32_MAX;
	min_measured_value[index].val2 = 0;
	max_measured_value[index].val1 = -INT32_MAX;
	max_measured_value[index].val2 = 0;
	min_range_value[index].val1 = 0;
	min_range_value[index].val2 = 0;
	max_range_value[index].val1 = 0;
	max_range_value[index].val2 = 0;

	/* initialize instance resource data */
	INIT_OBJ_RES(res[index], i, TEMP_SENSOR_VALUE_ID, 0,
		     &sensor_value[index], sizeof(*sensor_value),
		     NULL, NULL, sensor_value_write_cb, NULL);
	INIT_OBJ_RES_DATA(res[index], i, TEMP_UNITS_ID,
			  units[index], TEMP_STRING_SHORT);
	INIT_OBJ_RES_DATA(res[index], i, TEMP_MIN_MEASURED_VALUE_ID,
			  &min_measured_value[index],
			  sizeof(*min_measured_value));
	INIT_OBJ_RES_DATA(res[index], i, TEMP_MAX_MEASURED_VALUE_ID,
			  &max_measured_value[index],
			  sizeof(*max_measured_value));
	INIT_OBJ_RES_DATA(res[index], i, TEMP_MIN_RANGE_VALUE_ID,
			  &min_range_value[index],
			  sizeof(*min_range_value));
	INIT_OBJ_RES_DATA(res[index], i, TEMP_MAX_RANGE_VALUE_ID,
			  &max_range_value[index],
			  sizeof(*max_range_value));
	INIT_OBJ_RES_EXECUTE(res[index], i,
			     TEMP_RESET_MIN_MAX_MEASURED_VALUES_ID,
			     reset_min_max_measured_values_cb);

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	SYS_LOG_DBG("Create IPSO Temperature Sensor instance: %d",
		    obj_inst_id);
	return &inst[index];
}

static int ipso_temp_sensor_init(struct device *dev)
{
	int ret = 0;

	/* Set default values */
	(void)memset(inst, 0, sizeof(*inst) * MAX_INSTANCE_COUNT);
	(void)memset(res, 0, sizeof(struct lwm2m_engine_res_inst) *
			MAX_INSTANCE_COUNT * TEMP_MAX_ID);

	temp_sensor.obj_id = IPSO_OBJECT_TEMP_SENSOR_ID;
	temp_sensor.fields = fields;
	temp_sensor.field_count = ARRAY_SIZE(fields);
	temp_sensor.max_instance_count = MAX_INSTANCE_COUNT;
	temp_sensor.create_cb = temp_sensor_create;
	lwm2m_register_obj(&temp_sensor);

	return ret;
}

SYS_INIT(ipso_temp_sensor_init, APPLICATION,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
