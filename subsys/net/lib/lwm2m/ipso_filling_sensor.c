/*
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Filling sensor
 * https://github.com/OpenMobileAlliance/lwm2m-registry/blob/prod/3435.xml
 */

#define LOG_MODULE_NAME net_ipso_filling_sensor
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"
#include "ipso_filling_sensor.h"

#define FILLING_VERSION_MAJOR 1
#define FILLING_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_IPSO_FILLING_SENSOR_INSTANCE_COUNT

#define IPSO_OBJECT_ID IPSO_OBJECT_FILLING_LEVEL_SENSOR_ID

#define NUMBER_OF_OBJ_FIELDS 13

/*
 * Calculate resource instances as follows:
 * start with NUMBER_OF_OBJ_FIELDS
 * subtract EXEC resources (1)
 */
#define RESOURCE_INSTANCE_COUNT (NUMBER_OF_OBJ_FIELDS - 1)

/* resource state variables */
static uint32_t container_height[MAX_INSTANCE_COUNT]; /* cm */
static double actual_fill_percentage[MAX_INSTANCE_COUNT];
static uint32_t actual_fill_level[MAX_INSTANCE_COUNT]; /* cm */
static double high_threshold[MAX_INSTANCE_COUNT];
static bool container_full[MAX_INSTANCE_COUNT];
static double low_threshold[MAX_INSTANCE_COUNT];
static bool container_empty[MAX_INSTANCE_COUNT];
static double average_fill_speed[MAX_INSTANCE_COUNT];
static time_t forecast_full_date[MAX_INSTANCE_COUNT];
static time_t forecast_empty_date[MAX_INSTANCE_COUNT];
static bool container_out_of_location[MAX_INSTANCE_COUNT];
static bool container_out_of_position[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj fill_sensor;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(CONTAINER_HEIGHT_FILLING_SENSOR_RID, RW, U32),
	OBJ_FIELD_DATA(ACTUAL_FILL_PERCENTAGE_FILLING_SENSOR_RID, R_OPT,
		       FLOAT),
	OBJ_FIELD_DATA(ACTUAL_FILL_LEVEL_FILLING_SENSOR_RID, R_OPT, U32),
	OBJ_FIELD_DATA(HIGH_THRESHOLD_PERCENTAGE_FILLING_SENSOR_RID, RW_OPT,
		       FLOAT),
	OBJ_FIELD_DATA(CONTAINER_FULL_FILLING_SENSOR_RID, R, BOOL),
	OBJ_FIELD_DATA(LOW_THRESHOLD_PERCENTAGE_FILLING_SENSOR_RID, RW_OPT,
		       FLOAT),
	OBJ_FIELD_DATA(CONTAINER_EMPTY_FILLING_SENSOR_RID, R, BOOL),
	OBJ_FIELD_DATA(AVERAGE_FILL_SPEED_FILLING_SENSOR_RID, R_OPT, FLOAT),
	OBJ_FIELD_EXECUTE_OPT(RESET_AVERAGE_FILL_SPEED_FILLING_SENSOR_RID),
	OBJ_FIELD_DATA(FORECAST_FULL_DATE_FILLING_SENSOR_RID, R_OPT, TIME),
	OBJ_FIELD_DATA(FORECAST_EMPTY_DATE_FILLING_SENSOR_RID, R_OPT, TIME),
	OBJ_FIELD_DATA(CONTAINER_OUT_OF_LOCATION_FILLING_SENSOR_RID, R_OPT,
		       BOOL),
	OBJ_FIELD_DATA(CONTAINER_OUT_OF_POSITION_FILLING_SENSOR_RID, R_OPT,
		       BOOL),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][NUMBER_OF_OBJ_FIELDS];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT]
					    [RESOURCE_INSTANCE_COUNT];

static int reset_average_fill_speed_cb(uint16_t obj_inst_id, uint8_t *args,
				       uint16_t args_len)
{
	int i;

	LOG_DBG("Reset Average Fill Speed %d", obj_inst_id);
	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			average_fill_speed[i] = 0;
			return 0;
		}
	}

	return -ENOENT;
}

/* Update empty/full when fill percentage or thresholds change.
 * If value changes, then notify.
 */
static void update(uint16_t obj_inst_id, uint16_t res_id, int index)
{
	bool full;
	bool empty;

	full = actual_fill_percentage[index] > high_threshold[index];
	if (container_full[index] != full) {
		container_full[index] = full;
		lwm2m_notify_observer(IPSO_OBJECT_ID, obj_inst_id,
				CONTAINER_FULL_FILLING_SENSOR_RID);
	}

	empty = actual_fill_percentage[index] < low_threshold[index];
	if (container_empty[index] != empty) {
		container_empty[index] = empty;
		lwm2m_notify_observer(IPSO_OBJECT_ID, obj_inst_id,
				CONTAINER_EMPTY_FILLING_SENSOR_RID);
	}
}

static int update_cb(uint16_t obj_inst_id, uint16_t res_id,
		     uint16_t res_inst_id, uint8_t *data, uint16_t data_len,
		     bool last_block, size_t total_size)
{
	int i;

	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			update(obj_inst_id, res_id, i);
			break;
		}
	}
	return 0;
}

static struct lwm2m_engine_obj_inst *filling_sensor_create(uint16_t obj_inst_id)
{
	int index, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Can not create instance - "
				"already existing: %u",
				obj_inst_id);
			return NULL;
		}
	}

	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (!inst[index].obj) {
			break;
		}
	}

	if (index >= MAX_INSTANCE_COUNT) {
		LOG_ERR("Can not create instance - no more room: %u",
			obj_inst_id);
		return NULL;
	}

	/* Set default values (objects can be removed/recreated during runtime) */
	container_height[index] = 0;
	actual_fill_percentage[index] = 0;
	actual_fill_level[index] = 0;
	high_threshold[index] = 0;
	container_full[index] = false;
	low_threshold[index] = 0;
	container_empty[index] = false;
	average_fill_speed[index] = 0;
	forecast_full_date[index] = 0;
	forecast_empty_date[index] = 0;
	container_out_of_location[index] = false;
	container_out_of_position[index] = false;

	(void)memset(res[index], 0,
		     sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES(CONTAINER_HEIGHT_FILLING_SENSOR_RID, res[index], i,
		     res_inst[index], j, 1, false, true,
		     &container_height[index], sizeof(*container_height), NULL,
		     NULL, NULL, update_cb, NULL);
	INIT_OBJ_RES(ACTUAL_FILL_PERCENTAGE_FILLING_SENSOR_RID, res[index], i,
		     res_inst[index], j, 1, false, true,
		     &actual_fill_percentage[index], sizeof(*actual_fill_percentage),
		     NULL, NULL, NULL, update_cb, NULL);
	INIT_OBJ_RES_DATA(ACTUAL_FILL_LEVEL_FILLING_SENSOR_RID, res[index],
			  i, res_inst[index], j, &actual_fill_level[index],
			  sizeof(*actual_fill_level));
	INIT_OBJ_RES(HIGH_THRESHOLD_PERCENTAGE_FILLING_SENSOR_RID, res[index],
		     i, res_inst[index], j, 1, false, true,
		     &high_threshold[index], sizeof(*high_threshold), NULL,
		     NULL, NULL, update_cb, NULL);
	INIT_OBJ_RES_DATA(CONTAINER_FULL_FILLING_SENSOR_RID, res[index], i,
			  res_inst[index], j, &container_full[index],
			  sizeof(*container_full));
	INIT_OBJ_RES(LOW_THRESHOLD_PERCENTAGE_FILLING_SENSOR_RID, res[index], i,
		     res_inst[index], j, 1, false, true, &low_threshold[index],
		     sizeof(*low_threshold), NULL, NULL, NULL, update_cb, NULL);
	INIT_OBJ_RES_DATA(CONTAINER_EMPTY_FILLING_SENSOR_RID, res[index], i,
			  res_inst[index], j, &container_empty[index],
			  sizeof(*container_empty));
	INIT_OBJ_RES_DATA(AVERAGE_FILL_SPEED_FILLING_SENSOR_RID, res[index], i,
			  res_inst[index], j, &average_fill_speed[index],
			  sizeof(*average_fill_speed));
	INIT_OBJ_RES_EXECUTE(RESET_AVERAGE_FILL_SPEED_FILLING_SENSOR_RID,
			     res[index], i, reset_average_fill_speed_cb);
	INIT_OBJ_RES_DATA(FORECAST_FULL_DATE_FILLING_SENSOR_RID, res[index], i,
			  res_inst[index], j, &forecast_full_date[index],
			  sizeof(*forecast_full_date));
	INIT_OBJ_RES_DATA(FORECAST_EMPTY_DATE_FILLING_SENSOR_RID, res[index], i,
			  res_inst[index], j, &forecast_empty_date[index],
			  sizeof(*forecast_empty_date));
	INIT_OBJ_RES_DATA(CONTAINER_OUT_OF_LOCATION_FILLING_SENSOR_RID,
			  res[index], i, res_inst[index], j,
			  &container_out_of_location[index],
			  sizeof(*container_out_of_location));
	INIT_OBJ_RES_DATA(CONTAINER_OUT_OF_POSITION_FILLING_SENSOR_RID,
			  res[index], i, res_inst[index], j,
			  &container_out_of_position[index],
			  sizeof(*container_out_of_position));
	inst[index].resources = res[index];
	inst[index].resource_count = i;
	LOG_DBG("Created IPSO Filling Sensor instance: %d", obj_inst_id);
	return &inst[index];
}

static int fill_sensor_init(void)
{
	fill_sensor.obj_id = IPSO_OBJECT_ID;
	fill_sensor.version_major = FILLING_VERSION_MAJOR;
	fill_sensor.version_minor = FILLING_VERSION_MINOR;
	fill_sensor.is_core = false;
	fill_sensor.fields = fields;
	fill_sensor.field_count = ARRAY_SIZE(fields);
	fill_sensor.max_instance_count = MAX_INSTANCE_COUNT;
	fill_sensor.create_cb = filling_sensor_create;
	lwm2m_register_obj(&fill_sensor);

	return 0;
}

LWM2M_OBJ_INIT(fill_sensor_init);
