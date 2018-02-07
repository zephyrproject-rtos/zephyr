/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "lwm2m_obj_server"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>
#include <stdint.h>
#include <init.h>
#include <net/lwm2m.h>

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
	OBJ_FIELD_DATA(SERVER_SHORT_SERVER_ID, R, U16),
	OBJ_FIELD_DATA(SERVER_LIFETIME_ID, RW, U32),
	OBJ_FIELD_DATA(SERVER_DEFAULT_MIN_PERIOD_ID, RW, U32),
	OBJ_FIELD_DATA(SERVER_DEFAULT_MAX_PERIOD_ID, RW, U32),
	OBJ_FIELD_EXECUTE(SERVER_DISABLE_ID),
	OBJ_FIELD_DATA(SERVER_DISABLE_TIMEOUT_ID, RW, U32),
	OBJ_FIELD_DATA(SERVER_STORE_NOTIFY_ID, RW, U8),
	OBJ_FIELD_DATA(SERVER_TRANSPORT_BINDING_ID, RW, STRING),
	OBJ_FIELD_EXECUTE(SERVER_REG_UPDATE_TRIGGER_ID),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res_inst res[MAX_INSTANCE_COUNT][SERVER_MAX_ID];

#define BINDING_MODE_U		0
#define BINDING_MODE_UQ		1
#define BINDING_MODE_S		2
#define BINDING_MODE_SQ		3
#define BINDING_MODE_US		4
#define BINDING_MODE_UQS	5
#define MAX_BINDING_MODE	6

static const char *binding_modes[MAX_BINDING_MODE] = {
	"U", "UQ", "S", "SQ", "US", "UQS"
};

static char binding_mode_validation_buf[TRANSPORT_BINDING_LEN];

static int disable_cb(u16_t obj_inst_id)
{
	int i;

	SYS_LOG_DBG("DISABLE %d", obj_inst_id);
	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			server_flag_disabled[i] = 1;
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

/* Setup an alternate buffer for validating transport binding mode at
 * binding_validate() function
 */
void *binding_pre_write(u16_t obj_inst_id, size_t *data_len)
{
	*data_len = TRANSPORT_BINDING_LEN;
	return binding_mode_validation_buf;
}

static int binding_validate(u16_t obj_inst_id, u8_t *data, u16_t data_len,
			    bool last_block, size_t total_size)
{
	u16_t mode;

	for (mode = BINDING_MODE_U; mode < MAX_BINDING_MODE; mode++) {
		if (strlen(binding_modes[mode]) == data_len &&
		    !strcmp(binding_modes[mode], data)) {
			break;
		}
	}

	/* only UDP mode is supported for now */
	if (mode != BINDING_MODE_U) {
		SYS_LOG_ERR("Unsupported binding mode: %s",
			    mode == MAX_BINDING_MODE ?
			    "unknown" : binding_modes[mode]);
		return -ENOTSUP;
	}

	return 0;
}

/* After transport binding mode is validated by binding_validate(),
 * we have to write back the incoming data from alternate buffer setup by
 * binding_pre_write() function
 */
static int binding_post_write(u16_t obj_inst_id, u8_t *data, u16_t data_len,
			      bool last_block, size_t total_size)
{
	int i;

	if (data == NULL || data_len > TRANSPORT_BINDING_LEN) {
		return -EINVAL;
	}

	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			strcpy(transport_binding[i], data);
			return 0;
		}
	}

	return -ENOENT;
}

static struct lwm2m_engine_obj_inst *server_create(u16_t obj_inst_id)
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
	server_flag_disabled[index] = 0;
	server_flag_store_notify[index] = 0;
	server_id[index] = index + 1;
	lifetime[index] = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;
	default_min_period[index] = 0;
	default_max_period[index] = 0;
	disabled_timeout[index] = 86400;
	strcpy(transport_binding[index], binding_modes[BINDING_MODE_U]);

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(res[index], i, SERVER_SHORT_SERVER_ID,
			  &server_id[index], sizeof(*server_id));
	INIT_OBJ_RES_DATA(res[index], i, SERVER_LIFETIME_ID,
			  &lifetime[index], sizeof(*lifetime));
	INIT_OBJ_RES_DATA(res[index], i, SERVER_DEFAULT_MIN_PERIOD_ID,
			  &default_min_period[index],
			  sizeof(*default_min_period));
	INIT_OBJ_RES_DATA(res[index], i, SERVER_DEFAULT_MAX_PERIOD_ID,
			  &default_max_period[index],
			  sizeof(*default_max_period));
	INIT_OBJ_RES_EXECUTE(res[index], i, SERVER_DISABLE_ID, disable_cb);
	INIT_OBJ_RES_DATA(res[index], i, SERVER_DISABLE_TIMEOUT_ID,
			  &disabled_timeout[index],
			  sizeof(*disabled_timeout));
	INIT_OBJ_RES_DATA(res[index], i, SERVER_STORE_NOTIFY_ID,
			  &server_flag_store_notify[index],
			  sizeof(*server_flag_store_notify));
	INIT_OBJ_RES(res[index], i, SERVER_TRANSPORT_BINDING_ID, 0,
		     transport_binding[index], TRANSPORT_BINDING_LEN,
		     NULL, binding_pre_write, binding_post_write, NULL,
		     binding_validate);
	INIT_OBJ_RES_EXECUTE(res[index], i, SERVER_REG_UPDATE_TRIGGER_ID,
			  update_trigger_cb);

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	SYS_LOG_DBG("Create LWM2M server instance: %d", obj_inst_id);
	return &inst[index];
}

static int lwm2m_server_init(struct device *dev)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	/* Set default values */
	memset(inst, 0, sizeof(*inst) * MAX_INSTANCE_COUNT);
	memset(res, 0, sizeof(struct lwm2m_engine_res_inst) *
		       MAX_INSTANCE_COUNT * SERVER_MAX_ID);

	server.obj_id = LWM2M_OBJECT_SERVER_ID;
	server.fields = fields;
	server.field_count = sizeof(fields) / sizeof(*fields);
	server.max_instance_count = MAX_INSTANCE_COUNT;
	server.create_cb = server_create;
	lwm2m_register_obj(&server);

	/* auto create the first instance */
	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_SERVER_ID, 0, &obj_inst);
	if (ret < 0) {
		SYS_LOG_ERR("Create LWM2M server instance 0 error: %d", ret);
	}

	return ret;
}

SYS_INIT(lwm2m_server_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
