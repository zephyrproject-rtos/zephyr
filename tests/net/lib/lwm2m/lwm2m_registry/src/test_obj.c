/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define TEST_OBJ_ID 32768

bool b;
uint8_t opaque[32];
char string[32];
uint8_t u8;
int8_t s8;
uint16_t u16;
int16_t s16;
uint32_t u32;
int32_t s32;
uint64_t u64;
int64_t s64;
time_t t;
double d;
struct lwm2m_objlnk objl;

static struct lwm2m_engine_obj test_obj;
/* Use LWM2M_RES_TYPE_* also as a resource ID, so that
 * resource ID of U8 type is LWM2M_RES_TYPE_U8
 */
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD(LWM2M_RES_TYPE_OPAQUE, RW, OPAQUE),
	OBJ_FIELD(LWM2M_RES_TYPE_STRING, RW, STRING),
	OBJ_FIELD(LWM2M_RES_TYPE_U32, RW, U32),
	OBJ_FIELD(LWM2M_RES_TYPE_U16, RW, U16),
	OBJ_FIELD(LWM2M_RES_TYPE_U8, RW, U8),
	OBJ_FIELD(LWM2M_RES_TYPE_S64, RW, S64),
	OBJ_FIELD(LWM2M_RES_TYPE_S32, RW, S32),
	OBJ_FIELD(LWM2M_RES_TYPE_S16, RW, S16),
	OBJ_FIELD(LWM2M_RES_TYPE_S8, RW, S8),
	OBJ_FIELD(LWM2M_RES_TYPE_BOOL, RW, BOOL),
	OBJ_FIELD(LWM2M_RES_TYPE_TIME, RW, TIME),
	OBJ_FIELD(LWM2M_RES_TYPE_FLOAT, RW, FLOAT),
	OBJ_FIELD(LWM2M_RES_TYPE_OBJLNK, RW, OBJLNK)
};
#define RESOURCE_COUNT LWM2M_RES_TYPE_OBJLNK

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res res[RESOURCE_COUNT];
static struct lwm2m_engine_res_inst res_inst[RESOURCE_COUNT];

static struct lwm2m_engine_obj_inst *obj_create(uint16_t obj_inst_id)
{
	int i = 0, j = 0;
	static bool created;

	if (created || obj_inst_id != 0) {
		return NULL;
	}
	created = true;

	(void)memset(res, 0, sizeof(res));
	init_res_instance(res_inst, ARRAY_SIZE(res_inst));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA_LEN(LWM2M_RES_TYPE_OPAQUE, res, i, res_inst, j, opaque, sizeof(opaque),
			      0);
	INIT_OBJ_RES_DATA_LEN(LWM2M_RES_TYPE_STRING, res, i, res_inst, j, string, sizeof(string),
			      0);
	INIT_OBJ_RES_DATA(LWM2M_RES_TYPE_U32, res, i, res_inst, j, &u32, sizeof(u32));
	INIT_OBJ_RES_DATA(LWM2M_RES_TYPE_U16, res, i, res_inst, j, &u16, sizeof(u16));
	INIT_OBJ_RES_DATA(LWM2M_RES_TYPE_U8, res, i, res_inst, j, &u8, sizeof(u8));
	INIT_OBJ_RES_DATA(LWM2M_RES_TYPE_S64, res, i, res_inst, j, &s64, sizeof(s64));
	INIT_OBJ_RES_DATA(LWM2M_RES_TYPE_S32, res, i, res_inst, j, &s32, sizeof(s32));
	INIT_OBJ_RES_DATA(LWM2M_RES_TYPE_S16, res, i, res_inst, j, &s16, sizeof(s16));
	INIT_OBJ_RES_DATA(LWM2M_RES_TYPE_S8, res, i, res_inst, j, &s8, sizeof(s8));
	INIT_OBJ_RES_DATA(LWM2M_RES_TYPE_TIME, res, i, res_inst, j, &t, sizeof(t));
	INIT_OBJ_RES_DATA(LWM2M_RES_TYPE_BOOL, res, i, res_inst, j, &b, sizeof(b));
	INIT_OBJ_RES_DATA(LWM2M_RES_TYPE_FLOAT, res, i, res_inst, j, &d, sizeof(d));
	INIT_OBJ_RES_DATA(LWM2M_RES_TYPE_OBJLNK, res, i, res_inst, j, &objl, sizeof(objl));

	inst.resources = res;
	inst.resource_count = i;

	return &inst;
}

static int obj_init(void)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;

	test_obj.obj_id = TEST_OBJ_ID;
	test_obj.version_major = 1;
	test_obj.version_minor = 0;
	test_obj.is_core = false;
	test_obj.fields = fields;
	test_obj.field_count = ARRAY_SIZE(fields);
	test_obj.max_instance_count = 1;
	test_obj.create_cb = obj_create;
	lwm2m_register_obj(&test_obj);

	/* auto create the first instance */
	return lwm2m_create_obj_inst(TEST_OBJ_ID, 0, &obj_inst);
}

LWM2M_OBJ_INIT(obj_init);
