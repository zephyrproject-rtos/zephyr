/**
 * @file lwm2m_event_log.c
 * @brief
 *
 * Copyright (c) 2022 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Event Log
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/20.xml
 */

#define LOG_MODULE_NAME net_lwm2m_event_log
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_obj_event_log.h"

#define EVENT_LOG_VERSION_MAJOR 1
#define EVENT_LOG_VERSION_MINOR 0
#define EVENT_LOG_MAX_ID 6

/*
 * Calculate resource instances as follows:
 * start with EVENT_LOG_MAX_ID
 * subtract EXEC resources (2)
 */
#define RESOURCE_INSTANCE_COUNT (EVENT_LOG_MAX_ID - 2)

struct lwm2m_event_log_obj {
	uint8_t log_class;
	uint8_t log_status;
	uint8_t log_data_format;
};

static struct lwm2m_engine_obj lwm2m_event_log;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(LWM2M_EVENT_LOG_CLASS_ID, RW_OPT, U8),
	OBJ_FIELD_EXECUTE_OPT(LWM2M_EVENT_LOG_START_ID),
	OBJ_FIELD_EXECUTE_OPT(LWM2M_EVENT_LOG_STOP_ID),
	OBJ_FIELD_DATA(LWM2M_EVENT_LOG_STATUS_ID, R_OPT, U8),
	OBJ_FIELD_DATA(LWM2M_EVENT_LOG_DATA_ID, R, OPAQUE),
	OBJ_FIELD_DATA(LWM2M_EVENT_LOG_DATAFORMAT_ID, RW_OPT, U8),
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res res[EVENT_LOG_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *lwm2m_event_log_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;

	init_res_instance(res_inst, ARRAY_SIZE(res_inst));

	/* initialize instance resource data */
	INIT_OBJ_RES_OPTDATA(LWM2M_EVENT_LOG_CLASS_ID, res, i, res_inst, j);
	INIT_OBJ_RES_EXECUTE(LWM2M_EVENT_LOG_START_ID, res, i, NULL);
	INIT_OBJ_RES_EXECUTE(LWM2M_EVENT_LOG_STOP_ID, res, i, NULL);
	INIT_OBJ_RES_OPTDATA(LWM2M_EVENT_LOG_STATUS_ID, res, i, res_inst, j);
	INIT_OBJ_RES_OPT(LWM2M_EVENT_LOG_DATA_ID, res, i, res_inst, j, 1,
			 false, true, NULL, NULL, NULL, NULL, NULL);
	INIT_OBJ_RES_OPTDATA(LWM2M_EVENT_LOG_DATAFORMAT_ID, res, i, res_inst, j);

	inst.resources = res;
	inst.resource_count = i;

	LOG_DBG("Created LWM2M event log instance: %d", obj_inst_id);
	return &inst;
}

static int lwm2m_event_log_init(void)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	/* initialize the Event Log field data */
	lwm2m_event_log.obj_id = LWM2M_OBJECT_EVENT_LOG_ID;
	lwm2m_event_log.version_major = EVENT_LOG_VERSION_MAJOR;
	lwm2m_event_log.version_minor = EVENT_LOG_VERSION_MINOR;
	lwm2m_event_log.is_core = false;
	lwm2m_event_log.fields = fields;
	lwm2m_event_log.field_count = ARRAY_SIZE(fields);
	lwm2m_event_log.max_instance_count = 1U;
	lwm2m_event_log.create_cb = lwm2m_event_log_create;
	lwm2m_register_obj(&lwm2m_event_log);

	/* auto create the first instance */
	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_EVENT_LOG_ID, 0, &obj_inst);
	if (ret < 0) {
		LOG_ERR("Create LWM2M Event Log instance 0 error: %d", ret);
	}

	return ret;
}

LWM2M_OBJ_INIT(lwm2m_event_log_init);
