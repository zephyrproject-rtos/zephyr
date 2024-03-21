/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_obj_server
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_obj_server.h"
#include "lwm2m_rd_client.h"
#include "lwm2m_registry.h"
#include "lwm2m_engine.h"

#define SERVER_VERSION_MAJOR 1
#if defined(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1)
#define SERVER_VERSION_MINOR 1
#define SERVER_MAX_ID		 24
#else
#define SERVER_VERSION_MINOR 0
#define SERVER_MAX_ID		 9
#endif /* defined(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1) */

/* Server flags */
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
static uint16_t server_id[MAX_INSTANCE_COUNT];
static uint32_t lifetime[MAX_INSTANCE_COUNT];
static uint32_t default_min_period[MAX_INSTANCE_COUNT];
static uint32_t default_max_period[MAX_INSTANCE_COUNT];
static k_timepoint_t disabled_until[MAX_INSTANCE_COUNT];
static uint32_t disabled_timeout[MAX_INSTANCE_COUNT];
static uint8_t  server_flag_store_notify[MAX_INSTANCE_COUNT];
static char  transport_binding[MAX_INSTANCE_COUNT][TRANSPORT_BINDING_LEN];
/* Server object version 1.1 */
static uint8_t priority[MAX_INSTANCE_COUNT];
static bool mute_send[MAX_INSTANCE_COUNT];
static bool boostrap_on_fail[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj server;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(SERVER_SHORT_SERVER_ID, R, U16),
	OBJ_FIELD_DATA(SERVER_LIFETIME_ID, RW, U32),
	OBJ_FIELD_DATA(SERVER_DEFAULT_MIN_PERIOD_ID, RW_OPT, U32),
	OBJ_FIELD_DATA(SERVER_DEFAULT_MAX_PERIOD_ID, RW_OPT, U32),
	OBJ_FIELD_EXECUTE_OPT(SERVER_DISABLE_ID),
	OBJ_FIELD_DATA(SERVER_DISABLE_TIMEOUT_ID, RW_OPT, U32),
	OBJ_FIELD_DATA(SERVER_STORE_NOTIFY_ID, RW, BOOL),
	/* Mark Transport Binding is RO but BOOTSTRAP needs to write it */
	OBJ_FIELD_DATA(SERVER_TRANSPORT_BINDING_ID, RW, STRING),
	OBJ_FIELD_EXECUTE(SERVER_REG_UPDATE_TRIGGER_ID),
#if defined(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1)
	OBJ_FIELD_EXECUTE(SERVER_BOOTSTRAP_UPDATE_TRIGGER_ID),
	OBJ_FIELD_DATA(SERVER_APN_LINK_ID, RW_OPT, OBJLNK),
	OBJ_FIELD_DATA(SERVER_TLS_DTLS_ALERT_CODE_ID, R_OPT, U8),
	OBJ_FIELD_DATA(SERVER_LAST_BOOTSTRAPPED_ID, R_OPT, TIME),
	OBJ_FIELD_DATA(SERVER_REGISTRATION_PRIORITY_ORDER_ID, RW_OPT, U8),
	OBJ_FIELD_DATA(SERVER_INITIAL_REGISTRATION_DELAY_TIMER_ID, W_OPT, U16),
	OBJ_FIELD_DATA(SERVER_REGISTRATION_FAILURE_BLOCK_ID, W_OPT, BOOL),
	OBJ_FIELD_DATA(SERVER_BOOTSTRAP_ON_REGISTRATION_FAILURE_ID, RW_OPT, BOOL),
	OBJ_FIELD_DATA(SERVER_COMMUNICATION_RETRY_COUNT_ID, W_OPT, U16),
	OBJ_FIELD_DATA(SERVER_COMMUNICATION_RETRY_TIMER_ID, W_OPT, U16),
	OBJ_FIELD_DATA(SERVER_COMMUNICATION_SEQUENCE_DELAY_TIMER_ID, W_OPT, U16),
	OBJ_FIELD_DATA(SERVER_COMMUNICATION_SEQUENCE_RETRY_TIMER_ID, W_OPT, U16),
	OBJ_FIELD_DATA(SERVER_SMS_TRIGGER_ID, RW_OPT, BOOL),
	OBJ_FIELD_DATA(SERVER_PREFERRED_TRANSPORT_ID, RW_OPT, STRING),
	OBJ_FIELD_DATA(SERVER_MUTE_SEND_ID, RW_OPT, BOOL),
#endif /* defined(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1) */

};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][SERVER_MAX_ID];
static struct lwm2m_engine_res_inst
			res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static int disable_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	ARG_UNUSED(args);
	ARG_UNUSED(args_len);

	int ret;

	for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			LOG_DBG("DISABLE %d", obj_inst_id);
			ret = lwm2m_rd_client_server_disabled(obj_inst_id);
			if (ret == 0) {
				disabled_until[i] =
					sys_timepoint_calc(K_SECONDS(disabled_timeout[i]));
				return 0;
			}
			return ret;
		}
	}

	return -ENOENT;
}

static int update_trigger_cb(uint16_t obj_inst_id,
			     uint8_t *args, uint16_t args_len)
{
	engine_trigger_update(false);
	return 0;
}

static int bootstrap_trigger_cb(uint16_t obj_inst_id,
			     uint8_t *args, uint16_t args_len)
{
	return engine_trigger_bootstrap();
}

bool lwm2m_server_get_mute_send(uint16_t obj_inst_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			return mute_send[i];
		}
	}
	return false;
}


static int lifetime_write_cb(uint16_t obj_inst_id, uint16_t res_id,
			     uint16_t res_inst_id, uint8_t *data,
			     uint16_t data_len, bool last_block,
			     size_t total_size)
{
	ARG_UNUSED(obj_inst_id);
	ARG_UNUSED(res_id);
	ARG_UNUSED(res_inst_id);
	ARG_UNUSED(data);
	ARG_UNUSED(data_len);
	ARG_UNUSED(last_block);
	ARG_UNUSED(total_size);

	engine_trigger_update(false);
	return 0;
}

static int32_t server_get_instance_s32(uint16_t obj_inst_id, int32_t *data,
				     int32_t default_value)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			return data[i];
		}
	}

	return default_value;
}

int32_t lwm2m_server_get_pmin(uint16_t obj_inst_id)
{
	return server_get_instance_s32(obj_inst_id, default_min_period,
				       CONFIG_LWM2M_SERVER_DEFAULT_PMIN);
}

int32_t lwm2m_server_get_pmax(uint16_t obj_inst_id)
{
	return server_get_instance_s32(obj_inst_id, default_max_period,
				       CONFIG_LWM2M_SERVER_DEFAULT_PMAX);
}

int lwm2m_server_get_ssid(uint16_t obj_inst_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			return server_id[i];
		}
	}

	return -ENOENT;
}

int lwm2m_server_short_id_to_inst(uint16_t short_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		if (inst[i].obj && server_id[i] == short_id) {
			return inst[i].obj_inst_id;
		}
	}

	return -ENOENT;
}

static int lwm2m_server_inst_id_to_index(uint16_t obj_inst_id)
{
	for (int i = 0; i < ARRAY_SIZE(inst); i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			return i;
		}
	}
	return -1;
}

bool lwm2m_server_is_enabled(uint16_t obj_inst_id)
{
	int idx = lwm2m_server_inst_id_to_index(obj_inst_id);

	if (idx < 0) {
		return false;
	}
	return sys_timepoint_expired(disabled_until[idx]);
}

int lwm2m_server_disable(uint16_t obj_inst_id, k_timeout_t timeout)
{
	int idx = lwm2m_server_inst_id_to_index(obj_inst_id);

	if (idx < 0) {
		return -ENOENT;
	}
	disabled_until[idx] = sys_timepoint_calc(timeout);
	return 0;
}

k_timepoint_t lwm2m_server_get_disabled_time(uint16_t obj_inst_id)
{
	int idx = lwm2m_server_inst_id_to_index(obj_inst_id);

	if (idx < 0) {
		return sys_timepoint_calc(K_FOREVER);
	}
	return disabled_until[idx];
}

void lwm2m_server_reset_timestamps(void)
{
	for (int i = 0; i < ARRAY_SIZE(inst); i++) {
		disabled_until[i] = sys_timepoint_calc(K_NO_WAIT);
	}
}

bool lwm2m_server_select(uint16_t *obj_inst_id)
{
	uint8_t min = UINT8_MAX;
	uint8_t max = 0;

	/* Find priority boundaries */
	if (IS_ENABLED(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1)) {
		for (int i = 0; i < ARRAY_SIZE(inst); i++) {
			if (min > priority[i]) {
				min = priority[i];
			}
			if (max < priority[i]) {
				max = priority[i];
			}
		}
	} else  {
		min = max = 0;
	}

	for (uint8_t prio = min; prio <= max; prio++) {
		for (int i = 0; i < ARRAY_SIZE(inst); i++) {
			/* Disabled for a period */
			if (!lwm2m_server_is_enabled(inst[i].obj_inst_id)) {
				continue;
			}

			/* Invalid short IDs */
			if (server_id[i] == 0 || server_id[i] == UINT16_MAX) {
				continue;
			}

			/* Check priority */
			if (IS_ENABLED(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1)) {
				if (priority[i] > prio) {
					continue;
				}
			}
			if (obj_inst_id) {
				*obj_inst_id = inst[i].obj_inst_id;
			}
			return true;
		}
	}

	LOG_ERR("No server candidate found");
	return false;
}

uint8_t lwm2m_server_get_prio(uint16_t obj_inst_id)
{
	if (IS_ENABLED(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1)) {
		int idx = lwm2m_server_inst_id_to_index(obj_inst_id);

		if (idx < 0) {
			return UINT8_MAX;
		}
		return priority[idx];
	}

	return (uint8_t)obj_inst_id % UINT8_MAX;
}

static struct lwm2m_engine_obj_inst *server_create(uint16_t obj_inst_id)
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
	disabled_until[i] = sys_timepoint_calc(K_NO_WAIT);
	server_flag_store_notify[index] = 0U;
	server_id[index] = index + 1;
	lifetime[index] = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;
	default_min_period[index] = CONFIG_LWM2M_SERVER_DEFAULT_PMIN;
	default_max_period[index] = CONFIG_LWM2M_SERVER_DEFAULT_PMAX;
	disabled_timeout[index] = 86400U;
	boostrap_on_fail[index] = true;

	lwm2m_engine_get_binding(transport_binding[index]);

	(void)memset(res[index], 0,
		     sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(SERVER_SHORT_SERVER_ID, res[index], i,
			  res_inst[index], j,
			  &server_id[index], sizeof(*server_id));
	INIT_OBJ_RES(SERVER_LIFETIME_ID, res[index], i, res_inst[index], j,
		     1U, false, true, &lifetime[index], sizeof(*lifetime),
		     NULL, NULL, NULL, lifetime_write_cb, NULL);
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
	INIT_OBJ_RES_DATA_LEN(SERVER_TRANSPORT_BINDING_ID, res[index], i, res_inst[index], j,
			      transport_binding[index], TRANSPORT_BINDING_LEN,
			      strlen(transport_binding[index]) + 1);
	INIT_OBJ_RES_EXECUTE(SERVER_REG_UPDATE_TRIGGER_ID, res[index], i, update_trigger_cb);

	if (IS_ENABLED(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1)) {
		mute_send[index] = false;
		priority[index] = 0;
		INIT_OBJ_RES_EXECUTE(SERVER_BOOTSTRAP_UPDATE_TRIGGER_ID, res[index], i,
				     bootstrap_trigger_cb);
		INIT_OBJ_RES_OPTDATA(SERVER_APN_LINK_ID, res[index], i, res_inst[index], j);
		INIT_OBJ_RES_OPTDATA(SERVER_TLS_DTLS_ALERT_CODE_ID, res[index], i, res_inst[index],
				     j);
		INIT_OBJ_RES_OPTDATA(SERVER_LAST_BOOTSTRAPPED_ID, res[index], i, res_inst[index],
				     j);
		INIT_OBJ_RES_DATA(SERVER_REGISTRATION_PRIORITY_ORDER_ID, res[index], i,
				  res_inst[index], j, &priority[index], sizeof(uint8_t));
		INIT_OBJ_RES_OPTDATA(SERVER_INITIAL_REGISTRATION_DELAY_TIMER_ID, res[index], i,
				     res_inst[index], j);
		INIT_OBJ_RES_OPTDATA(SERVER_REGISTRATION_FAILURE_BLOCK_ID, res[index], i,
				     res_inst[index], j);
		INIT_OBJ_RES_DATA(SERVER_BOOTSTRAP_ON_REGISTRATION_FAILURE_ID, res[index], i,
				  res_inst[index], j, &boostrap_on_fail[index], sizeof(bool));
		INIT_OBJ_RES_OPTDATA(SERVER_COMMUNICATION_RETRY_COUNT_ID, res[index], i,
				     res_inst[index], j);
		INIT_OBJ_RES_OPTDATA(SERVER_COMMUNICATION_RETRY_TIMER_ID, res[index], i,
				     res_inst[index], j);
		INIT_OBJ_RES_OPTDATA(SERVER_COMMUNICATION_SEQUENCE_DELAY_TIMER_ID, res[index], i,
				     res_inst[index], j);
		INIT_OBJ_RES_OPTDATA(SERVER_COMMUNICATION_SEQUENCE_RETRY_TIMER_ID, res[index], i,
				     res_inst[index], j);
		INIT_OBJ_RES_OPTDATA(SERVER_SMS_TRIGGER_ID, res[index], i, res_inst[index], j);
		INIT_OBJ_RES_OPTDATA(SERVER_PREFERRED_TRANSPORT_ID, res[index], i, res_inst[index],
				     j);
		INIT_OBJ_RES_DATA(SERVER_MUTE_SEND_ID, res[index], i, res_inst[index], j,
				  &mute_send[index], sizeof(bool));
	}

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	LOG_DBG("Create LWM2M server instance: %d", obj_inst_id);
	return &inst[index];
}

static int lwm2m_server_init(void)
{
	int ret = 0;

	server.obj_id = LWM2M_OBJECT_SERVER_ID;
	server.version_major = SERVER_VERSION_MAJOR;
	server.version_minor = SERVER_VERSION_MINOR;
	server.is_core = true;
	server.fields = fields;
	server.field_count = ARRAY_SIZE(fields);
	server.max_instance_count = MAX_INSTANCE_COUNT;
	server.create_cb = server_create;
	lwm2m_register_obj(&server);

	/* don't create automatically when using bootstrap */
	if (!IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)) {
		struct lwm2m_engine_obj_inst *obj_inst = NULL;

		ret = lwm2m_create_obj_inst(LWM2M_OBJECT_SERVER_ID, 0, &obj_inst);
		if (ret < 0) {
			LOG_ERR("Create LWM2M server instance 0 error: %d", ret);
		}
	}

	return ret;
}

LWM2M_CORE_INIT(lwm2m_server_init);
