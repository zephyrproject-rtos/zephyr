/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_obj_server
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#ifdef CONFIG_LWM2M_RD_CLIENT_SUPPORT
#include "lwm2m_rd_client.h"
#endif

/* Server resource IDs */
#define SERVER_SHORT_SERVER_ID		0
#define SERVER_LIFETIME_ID		1
#define SERVER_DEFAULT_MIN_PERIOD_ID	2
#define SERVER_DEFAULT_MAX_PERIOD_ID	3
#define SERVER_DISABLE_ID		4
#define SERVER_DISABLE_TIMEOUT_ID	5
#define SERVER_STORE_NOTIFY_ID		6
#define SERVER_TRANSPORT_BINDING_ID	7
#define SERVER_REG_UPDATE_TRIGGER_ID	8

#define SERVER_MAX_ID			9

/* Server flags */
#define SERVER_FLAG_DISABLED		1
#define SERVER_FLAG_STORE_NOTIFY	2

#define MAX_INSTANCE_COUNT		CONFIG_LWM2M_SERVER_INSTANCE_COUNT

#define TRANSPORT_BINDING_LEN		4

/*
 * Calculate resource instances as follows:
 * start with SERVER_MAX_ID
 * subtract EXEC resources (2)
 */
#define RESOURCE_INSTANCE_COUNT	(SERVER_MAX_ID - 2)

/* resource state variables */
static u16_t server_id[MAX_INSTANCE_COUNT];
static u32_t lifetime[MAX_INSTANCE_COUNT];
static u32_t default_min_period[MAX_INSTANCE_COUNT];
static u32_t default_max_period[MAX_INSTANCE_COUNT];
static u8_t  server_flag_disabled[MAX_INSTANCE_COUNT];
static u32_t disabled_timeout[MAX_INSTANCE_COUNT];
static u8_t  server_flag_store_notify[MAX_INSTANCE_COUNT];
static char  transport_binding[MAX_INSTANCE_COUNT][TRANSPORT_BINDING_LEN];

static struct lwm2m_engine_obj server;
static struct lwm2m_engine_obj_field fields[] = {
	/*
	 * LwM2M TS "E.2 LwM2M Object: LwM2M Server" page 107, describes
	 * Short Server ID as READ-ONLY, but BOOTSTRAP server will attempt
	 * to write it, so it needs to be RW
	 */
	OBJ_FIELD_DATA(SERVER_SHORT_SERVER_ID, RW, U16),
	OBJ_FIELD_DATA(SERVER_LIFETIME_ID, RW, U32),
	OBJ_FIELD_DATA(SERVER_DEFAULT_MIN_PERIOD_ID, RW_OPT, U32),
	OBJ_FIELD_DATA(SERVER_DEFAULT_MAX_PERIOD_ID, RW_OPT, U32),
	OBJ_FIELD_EXECUTE_OPT(SERVER_DISABLE_ID),
	OBJ_FIELD_DATA(SERVER_DISABLE_TIMEOUT_ID, RW_OPT, U32),
	OBJ_FIELD_DATA(SERVER_STORE_NOTIFY_ID, RW, U8),
	/* Mark Transport Binding is RO but BOOTSTRAP needs to write it */
	OBJ_FIELD_DATA(SERVER_TRANSPORT_BINDING_ID, RW, STRING),
	OBJ_FIELD_EXECUTE(SERVER_REG_UPDATE_TRIGGER_ID),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][SERVER_MAX_ID];
static struct lwm2m_engine_res_inst
			res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static int disable_cb(u16_t obj_inst_id)
{
	int i;

	LOG_DBG("DISABLE %d", obj_inst_id);
	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			server_flag_disabled[i] = 1U;
			return 0;
		}
	}

	return -ENOENT;
}

static int update_trigger_cb(u16_t obj_inst_id)
{
#ifdef CONFIG_LWM2M_RD_CLIENT_SUPPORT
	engine_trigger_update();
	return 0;
#else
	return -EPERM;
#endif
}

static s32_t server_get_instance_s32(u16_t obj_inst_id, s32_t *data,
				     s32_t default_value)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			return data[i];
		}
	}

	return default_value;
}

s32_t lwm2m_server_get_pmin(u16_t obj_inst_id)
{
	return server_get_instance_s32(obj_inst_id, default_min_period,
				       CONFIG_LWM2M_SERVER_DEFAULT_PMIN);
}

s32_t lwm2m_server_get_pmax(u16_t obj_inst_id)
{
	return server_get_instance_s32(obj_inst_id, default_max_period,
				       CONFIG_LWM2M_SERVER_DEFAULT_PMAX);
}

static struct lwm2m_engine_obj_inst *server_create(u16_t obj_inst_id)
{
	int index, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Can not create instance - "
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
		LOG_ERR("Can not create instance - "
			"no more room: %u", obj_inst_id);
		return NULL;
	}

	/* Set default values */
	server_flag_disabled[index] = 0U;
	server_flag_store_notify[index] = 0U;
	server_id[index] = index + 1;
	lifetime[index] = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;
	default_min_period[index] = CONFIG_LWM2M_SERVER_DEFAULT_PMIN;
	default_max_period[index] = CONFIG_LWM2M_SERVER_DEFAULT_PMAX;
	disabled_timeout[index] = 86400U;
	strcpy(transport_binding[index], "U");

	(void)memset(res[index], 0,
		     sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(SERVER_SHORT_SERVER_ID, res[index], i,
			  res_inst[index], j,
			  &server_id[index], sizeof(*server_id));
	INIT_OBJ_RES_DATA(SERVER_LIFETIME_ID, res[index], i,
			  res_inst[index], j,
			  &lifetime[index], sizeof(*lifetime));
	INIT_OBJ_RES_DATA(SERVER_DEFAULT_MIN_PERIOD_ID, res[index], i,
			  res_inst[index], j,
			  &default_min_period[index],
			  sizeof(*default_min_period));
	INIT_OBJ_RES_DATA(SERVER_DEFAULT_MAX_PERIOD_ID, res[index], i,
			  res_inst[index], j,
			  &default_max_period[index],
			  sizeof(*default_max_period));
	INIT_OBJ_RES_EXECUTE(SERVER_DISABLE_ID, res[index], i, disable_cb);
	INIT_OBJ_RES_DATA(SERVER_DISABLE_TIMEOUT_ID, res[index], i,
			  res_inst[index], j,
			  &disabled_timeout[index],
			  sizeof(*disabled_timeout));
	INIT_OBJ_RES_DATA(SERVER_STORE_NOTIFY_ID, res[index], i,
			  res_inst[index], j,
			  &server_flag_store_notify[index],
			  sizeof(*server_flag_store_notify));
	/* Mark Transport Binding RO as we only support UDP atm */
	INIT_OBJ_RES_DATA(SERVER_TRANSPORT_BINDING_ID, res[index], i,
			  res_inst[index], j,
			  transport_binding[index], TRANSPORT_BINDING_LEN);
	INIT_OBJ_RES_EXECUTE(SERVER_REG_UPDATE_TRIGGER_ID, res[index], i,
			     update_trigger_cb);

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	LOG_DBG("Create LWM2M server instance: %d", obj_inst_id);
	return &inst[index];
}

static int lwm2m_server_init(struct device *dev)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	server.obj_id = LWM2M_OBJECT_SERVER_ID;
	server.fields = fields;
	server.field_count = ARRAY_SIZE(fields);
	server.max_instance_count = MAX_INSTANCE_COUNT;
	server.create_cb = server_create;
	lwm2m_register_obj(&server);

	/* auto create the first instance */
	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_SERVER_ID, 0, &obj_inst);
	if (ret < 0) {
		LOG_ERR("Create LWM2M server instance 0 error: %d", ret);
	}

	return ret;
}

SYS_INIT(lwm2m_server_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
