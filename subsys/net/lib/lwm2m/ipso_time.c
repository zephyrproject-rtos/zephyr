/*
 * Copyright (c) 2026 Savo Saicic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for IPSO Time object (3333):
 * https://raw.githubusercontent.com/OpenMobileAlliance/lwm2m-registry/prod/3333.xml
 */

#define LOG_MODULE_NAME net_ipso_time
#define LOG_LEVEL       CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <time.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"

#define TIME_VERSION_MAJOR 1
#define TIME_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT      CONFIG_LWM2M_IPSO_TIMER_INSTANCE_COUNT
#define RESOURCE_INSTANCE_COUNT 3

/* resource state */
struct ipso_time_data {
	time_t current_time;
};

static struct ipso_time_data time_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj ipso_time;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(CURRENT_TIME_RID, RW, TIME),
	OBJ_FIELD_DATA(FRACTIONAL_TIME_RID, RW_OPT, FLOAT),
	OBJ_FIELD_DATA(APPLICATION_TYPE_RID, RW_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *time_inst_create(uint16_t obj_inst_id)
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

	/* Set default values */
	(void)memset(&time_data[avail], 0, sizeof(time_data[avail]));
	(void)memset(res[avail], 0, sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));
	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(CURRENT_TIME_RID, res[avail], i, res_inst[avail], j,
			  &time_data[avail].current_time, sizeof(time_data[avail].current_time));
	INIT_OBJ_RES_OPTDATA(FRACTIONAL_TIME_RID, res[avail], i, res_inst[avail], j);
	INIT_OBJ_RES_OPTDATA(APPLICATION_TYPE_RID, res[avail], i, res_inst[avail], j);

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Time instance: %d", obj_inst_id);

	return &inst[avail];
}

static int ipso_time_init(void)
{
	ipso_time.obj_id = IPSO_OBJECT_TIME_ID;
	ipso_time.version_major = TIME_VERSION_MAJOR;
	ipso_time.version_minor = TIME_VERSION_MINOR;
	ipso_time.is_core = false;
	ipso_time.fields = fields;
	ipso_time.field_count = ARRAY_SIZE(fields);
	ipso_time.max_instance_count = ARRAY_SIZE(inst);
	ipso_time.create_cb = time_inst_create;
	lwm2m_register_obj(&ipso_time);

	return 0;
}

LWM2M_OBJ_INIT(ipso_time_init);
