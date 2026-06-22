/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_obj_portfolio
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"

#define PORTFOLIO_VERSION_MAJOR 1
#define PORTFOLIO_VERSION_MINOR 0

#define PORTFOLIO_IDENTITY_ID			0
#define PORTFOLIO_GET_AUTH_DATA_ID		1
#define PORTFOLIO_AUTH_DATA_ID			2
#define PORTFOLIO_AUTH_STATUS_ID		3

#define PORTFOLIO_MAX_ID 4

/* Identity max based on LwM2M v1.1 conformance test requirements */
#define PORTFOLIO_IDENTITY_MAX 4
#define PORTFOLIO_AUTH_DATA_MAX 4
/* Support 2 multi instance object */
#define MAX_INSTANCE_COUNT	2

/* Default Identity buffer length 40 */
#define DEFAULT_IDENTITY_BUFFER_LENGTH 40

/*
 * Calculate resource instances as follows:
 * start with PORTFOLIO_MAX_ID
 * subtract MULTI resources and execute because their counts include 0 resource (3)
 * add PORTFOLIO_IDENTITY_ID resource instances
 * add PORTFOLIO_AUTH_DATA_ID resource instances
 */
#define RESOURCE_INSTANCE_COUNT        (PORTFOLIO_MAX_ID - 3 + \
					PORTFOLIO_IDENTITY_MAX + \
					PORTFOLIO_AUTH_DATA_MAX)

static struct lwm2m_engine_obj portfolio;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(PORTFOLIO_IDENTITY_ID, RW, STRING), /* Mandatory, multi-instance */
	OBJ_FIELD_EXECUTE(PORTFOLIO_GET_AUTH_DATA_ID), /* Optional, single-instance */
	OBJ_FIELD_DATA(PORTFOLIO_AUTH_DATA_ID, R_OPT, OPAQUE), /* Optional, multi-instance */
	OBJ_FIELD_DATA(PORTFOLIO_AUTH_STATUS_ID, R_OPT, U8) /* Optional, single-instance*/
};


static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][PORTFOLIO_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];
static char identity[MAX_INSTANCE_COUNT][PORTFOLIO_IDENTITY_MAX][DEFAULT_IDENTITY_BUFFER_LENGTH];

static struct lwm2m_engine_obj_inst *portfolio_create(uint16_t obj_inst_id)
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
	(void)memset(identity[avail], 0, DEFAULT_IDENTITY_BUFFER_LENGTH);

	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES_MULTI_DATA_LEN(PORTFOLIO_IDENTITY_ID, res[avail], i, res_inst[avail], j,
				    PORTFOLIO_IDENTITY_MAX, false, identity[avail],
				    DEFAULT_IDENTITY_BUFFER_LENGTH, 0);
	INIT_OBJ_RES_EXECUTE(PORTFOLIO_GET_AUTH_DATA_ID, res[avail], i, NULL);
	INIT_OBJ_RES_MULTI_OPTDATA(PORTFOLIO_AUTH_DATA_ID, res[avail], i, res_inst[avail], j,
				   PORTFOLIO_AUTH_DATA_MAX, false);
	INIT_OBJ_RES_OPTDATA(PORTFOLIO_AUTH_STATUS_ID, res[avail], i, res_inst[avail], j);

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create LwM2M Portfolio instance: %d", obj_inst_id);

	return &inst[avail];
}

static int lwm2m_portfolio_init(void)
{
	portfolio.obj_id = LWM2M_OBJECT_PORTFOLIO_ID;
	portfolio.version_major = PORTFOLIO_VERSION_MAJOR;
	portfolio.version_minor = PORTFOLIO_VERSION_MINOR;
	portfolio.is_core = false;
	portfolio.fields = fields;
	portfolio.field_count = ARRAY_SIZE(fields);
	portfolio.max_instance_count = MAX_INSTANCE_COUNT;
	portfolio.create_cb = portfolio_create;
	lwm2m_register_obj(&portfolio);

	return 0;
}

LWM2M_OBJ_INIT(lwm2m_portfolio_init);
