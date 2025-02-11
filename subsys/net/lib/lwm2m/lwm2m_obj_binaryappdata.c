/**
 * @file lwm2m_obj_binaryappdata.c
 * @brief
 *
 * Copyright (c) 2022 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * BinaryAppDataContainer
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/19.xml
 */

#define LOG_MODULE_NAME net_lwm2m_binaryappdata
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_obj_binaryappdata.h"

#define BINARYAPPDATA_VERSION_MAJOR 1
#define BINARYAPPDATA_VERSION_MINOR 0
#define BINARYAPPDATA_MAX_ID 6

/* Support 2 instances of binary data in one object */
#define BINARYAPPDATA_DATA_INSTANCE_MAX 2

/* Support 2 multi instance object */
#define MAX_INSTANCE_COUNT	2

/*
 * Calculate resource instances as follows:
 * start with BINARYAPPDATA_MAX_ID
 * subtract EXEC resources (0)
 * add BINARYAPPDATA_DATA_INSTANCE_MAX resource instances
 */
#define RESOURCE_INSTANCE_COUNT (BINARYAPPDATA_MAX_ID - (0) + BINARYAPPDATA_DATA_INSTANCE_MAX)

static struct lwm2m_engine_obj lwm2m_binaryappdata;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(LWM2M_BINARYAPPDATA_DATA_ID, RW, OPAQUE),
	OBJ_FIELD_DATA(LWM2M_BINARYAPPDATA_DATA_PRIORITY_ID, RW_OPT, U8),
	OBJ_FIELD_DATA(LWM2M_BINARYAPPDATA_DATA_CREATION_TIME_ID, RW_OPT, TIME),
	OBJ_FIELD_DATA(LWM2M_BINARYAPPDATA_DATA_DESCRIPTION_ID, RW_OPT, STRING),
	OBJ_FIELD_DATA(LWM2M_BINARYAPPDATA_DATA_FORMAT_ID, RW_OPT, STRING),
	OBJ_FIELD_DATA(LWM2M_BINARYAPPDATA_APP_ID, RW_OPT, U16),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][BINARYAPPDATA_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *lwm2m_binaryappdata_create(uint16_t obj_inst_id)
{
	int index, avail = -1, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < ARRAY_SIZE(inst); index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Can not create instance - "
				"already existing: %u",
				obj_inst_id);
			return NULL;
		}

		/* Save first available slot index */
		if (avail < 0 && !inst[index].obj) {
			avail = index;
		}
	}

	if (avail < 0) {
		LOG_ERR("Can not create instance - no more room: %u", obj_inst_id);
		return NULL;
	}

	(void)memset(res[avail], 0, sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));

	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES_OPT(LWM2M_BINARYAPPDATA_DATA_ID, res[avail], i, res_inst[avail], j,
			BINARYAPPDATA_DATA_INSTANCE_MAX, true, true, NULL, NULL, NULL, NULL, NULL);
	INIT_OBJ_RES_OPTDATA(LWM2M_BINARYAPPDATA_DATA_PRIORITY_ID, res[avail], i,
			res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(LWM2M_BINARYAPPDATA_DATA_CREATION_TIME_ID, res[avail], i,
			res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(LWM2M_BINARYAPPDATA_DATA_DESCRIPTION_ID, res[avail], i,
			res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(LWM2M_BINARYAPPDATA_DATA_FORMAT_ID, res[avail], i, res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(LWM2M_BINARYAPPDATA_APP_ID, res[avail], i, res_inst[avail], j);

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Created LWM2M binary app data container instance: %d", obj_inst_id);
	return &inst[avail];
}

static int lwm2m_binaryappdata_init(void)
{
	int ret = 0;

	/* initialize the Event Log field data */
	lwm2m_binaryappdata.obj_id = LWM2M_OBJECT_BINARYAPPDATACONTAINER_ID;
	lwm2m_binaryappdata.version_major = BINARYAPPDATA_VERSION_MAJOR;
	lwm2m_binaryappdata.version_minor = BINARYAPPDATA_VERSION_MINOR;
	lwm2m_binaryappdata.is_core = false;
	lwm2m_binaryappdata.fields = fields;
	lwm2m_binaryappdata.field_count = ARRAY_SIZE(fields);
	lwm2m_binaryappdata.max_instance_count = MAX_INSTANCE_COUNT;
	lwm2m_binaryappdata.create_cb = lwm2m_binaryappdata_create;
	lwm2m_register_obj(&lwm2m_binaryappdata);

	return ret;
}

LWM2M_OBJ_INIT(lwm2m_binaryappdata_init);
