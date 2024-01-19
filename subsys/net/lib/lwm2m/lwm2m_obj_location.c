/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_obj_location
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define LOCATION_VERSION_MAJOR 1
#define LOCATION_VERSION_MINOR 0

/* resource IDs */
#define LOCATION_LATITUDE_ID			0
#define LOCATION_LONGITUDE_ID			1
#define LOCATION_ALTITUDE_ID			2
#define LOCATION_RADIUS_ID			3
#define LOCATION_VELOCITY_ID			4
#define LOCATION_TIMESTAMP_ID			5
#define LOCATION_SPEED_ID			6

#define LOCATION_MAX_ID				7

/*
 * Calculate resource instances as follows:
 * start with LOCATION_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT	(LOCATION_MAX_ID)

/* resource state */
static double latitude;
static double longitude;
static double altitude;
static double radius;
static double speed;
static time_t timestamp;

static struct lwm2m_engine_obj location;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(LOCATION_LATITUDE_ID, R, FLOAT),
	OBJ_FIELD_DATA(LOCATION_LONGITUDE_ID, R, FLOAT),
	OBJ_FIELD_DATA(LOCATION_ALTITUDE_ID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(LOCATION_RADIUS_ID, R_OPT, FLOAT),
	OBJ_FIELD_DATA(LOCATION_VELOCITY_ID, R_OPT, OPAQUE),
	OBJ_FIELD_DATA(LOCATION_TIMESTAMP_ID, R, TIME),
	OBJ_FIELD_DATA(LOCATION_SPEED_ID, R_OPT, FLOAT),
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res res[LOCATION_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *location_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	if (inst.resource_count) {
		LOG_ERR("Only 1 instance of Location object can exist.");
		return NULL;
	}

	init_res_instance(res_inst, ARRAY_SIZE(res_inst));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(LOCATION_LATITUDE_ID, res, i, res_inst, j,
			  &latitude, sizeof(latitude));
	INIT_OBJ_RES_DATA(LOCATION_LONGITUDE_ID, res, i, res_inst, j,
			  &longitude, sizeof(longitude));
	INIT_OBJ_RES_DATA(LOCATION_ALTITUDE_ID, res, i, res_inst, j,
			  &altitude, sizeof(altitude));
	INIT_OBJ_RES_DATA(LOCATION_RADIUS_ID, res, i, res_inst, j,
			  &radius, sizeof(radius));
	INIT_OBJ_RES_OPTDATA(LOCATION_VELOCITY_ID, res, i, res_inst, j);
	INIT_OBJ_RES_DATA(LOCATION_TIMESTAMP_ID, res, i, res_inst, j,
			  &timestamp, sizeof(timestamp));
	INIT_OBJ_RES_DATA(LOCATION_SPEED_ID, res, i, res_inst, j,
			  &speed, sizeof(speed));

	inst.resources = res;
	inst.resource_count = i;

	LOG_DBG("Create Location instance: %d", obj_inst_id);

	return &inst;
}

static int ipso_location_init(void)
{
	int ret;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;

	location.obj_id = LWM2M_OBJECT_LOCATION_ID;
	location.version_major = LOCATION_VERSION_MAJOR;
	location.version_minor = LOCATION_VERSION_MINOR;
	location.is_core = true;
	location.fields = fields;
	location.field_count = ARRAY_SIZE(fields);
	location.max_instance_count = 1U;
	location.create_cb = location_create;
	lwm2m_register_obj(&location);

	/* auto create the only instance */
	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_LOCATION_ID, 0, &obj_inst);
	if (ret < 0) {
		LOG_DBG("Create LWM2M instance 0 error: %d", ret);
	}

	return ret;
}

LWM2M_CORE_INIT(ipso_location_init);
