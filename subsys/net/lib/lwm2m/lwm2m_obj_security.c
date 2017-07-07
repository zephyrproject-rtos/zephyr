/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "lwm2m_obj_security"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>
#include <stdint.h>
#include <init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

/* Security resource IDs */
#define SECURITY_SERVER_URI_ID		0
#define SECURITY_BOOTSTRAP_FLAG_ID	1
#define SECURITY_MODE_ID		2
#define SECURITY_CLIENT_PK_ID		3
#define SECURITY_SERVER_PK_ID		4
#define SECURITY_SECRET_KEY_ID		5
#define SECURITY_SHORT_SERVER_ID	10

#define SECURITY_MAX_ID			7

#define MAX_INSTANCE_COUNT		CONFIG_LWM2M_SECURITY_INSTANCE_COUNT

#define SECURITY_URI_LEN		255

/* resource state variables */
static char  security_uri[MAX_INSTANCE_COUNT][SECURITY_URI_LEN];
static bool  bootstrap_flag[MAX_INSTANCE_COUNT];
static u8_t  security_mode[MAX_INSTANCE_COUNT];
static u16_t short_server_id[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj security;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(SECURITY_SERVER_URI_ID, RW, STRING),
	OBJ_FIELD_DATA(SECURITY_BOOTSTRAP_FLAG_ID, W, BOOL),
	OBJ_FIELD_DATA(SECURITY_MODE_ID, W, U8),
	OBJ_FIELD_DATA(SECURITY_CLIENT_PK_ID, W, OPAQUE),	/* TODO */
	OBJ_FIELD_DATA(SECURITY_SERVER_PK_ID, W, OPAQUE),	/* TODO */
	OBJ_FIELD_DATA(SECURITY_SECRET_KEY_ID, W, OPAQUE),	/* TODO */
	OBJ_FIELD_DATA(SECURITY_SHORT_SERVER_ID, W, U16)	/* TODO */
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res_inst res[MAX_INSTANCE_COUNT][SECURITY_MAX_ID];

static struct lwm2m_engine_obj_inst *security_create(u16_t obj_inst_id)
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

	/* default values */
	security_uri[index][0] = '\0';
	bootstrap_flag[index] = 0;
	security_mode[index] = 0;
	short_server_id[index] = 0;

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(res[index], i, SECURITY_SERVER_URI_ID,
			  security_uri[index], SECURITY_URI_LEN);
	INIT_OBJ_RES_DATA(res[index], i, SECURITY_BOOTSTRAP_FLAG_ID,
			  &bootstrap_flag[index], sizeof(*bootstrap_flag));
	INIT_OBJ_RES_DATA(res[index], i, SECURITY_MODE_ID,
			  &security_mode[index], sizeof(*security_mode));
	/* TODO: */
	INIT_OBJ_RES_DUMMY(res[index], i, SECURITY_CLIENT_PK_ID);
	INIT_OBJ_RES_DUMMY(res[index], i, SECURITY_SERVER_PK_ID),
	INIT_OBJ_RES_DUMMY(res[index], i, SECURITY_SECRET_KEY_ID),
	INIT_OBJ_RES_DATA(res[index], i, SECURITY_SHORT_SERVER_ID,
			  &short_server_id[index], sizeof(*short_server_id));

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	SYS_LOG_DBG("Create LWM2M security instance: %d", obj_inst_id);
	return &inst[index];
}

static int lwm2m_security_init(struct device *dev)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	/* Set default values */
	memset(inst, 0, sizeof(*inst) * MAX_INSTANCE_COUNT);
	memset(res, 0, sizeof(struct lwm2m_engine_res_inst) *
		       MAX_INSTANCE_COUNT * SECURITY_MAX_ID);

	security.obj_id = LWM2M_OBJECT_SECURITY_ID;
	security.fields = fields;
	security.field_count = sizeof(fields) / sizeof(*fields);
	security.max_instance_count = MAX_INSTANCE_COUNT;
	security.create_cb = security_create;
	lwm2m_register_obj(&security);

	/* auto create the first instance */
	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_SECURITY_ID, 0, &obj_inst);
	if (ret < 0) {
		SYS_LOG_ERR("Create LWM2M security instance 0 error: %d", ret);
	}

	return ret;
}

SYS_INIT(lwm2m_security_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
