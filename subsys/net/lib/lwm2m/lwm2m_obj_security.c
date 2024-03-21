/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_obj_security
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define SECURITY_VERSION_MAJOR 1
#if defined(CONFIG_LWM2M_SECURITY_OBJECT_VERSION_1_1)
#define SECURITY_VERSION_MINOR 1
#define SECURITY_MAX_ID 18
#ifdef CONFIG_LWM2M_SECURITY_DTLS_TLS_CIPHERSUITE_MAX
#define DTLS_TLS_CIPHERSUITE_MAX CONFIG_LWM2M_SECURITY_DTLS_TLS_CIPHERSUITE_MAX
#else
#define DTLS_TLS_CIPHERSUITE_MAX 0
#endif /* CONFIG_LWM2M_SECURITY_DTLS_TLS_CIPHERSUITE_MAX */
#else
#define SECURITY_VERSION_MINOR 0
#define SECURITY_MAX_ID 13
#endif /* defined(CONFIG_LWM2M_SECURITY_OBJECT_VERSION_1_1) */

/* Security resource IDs */
#define SECURITY_SERVER_URI_ID			0
#define SECURITY_BOOTSTRAP_FLAG_ID		1
#define SECURITY_MODE_ID			2
#define SECURITY_CLIENT_PK_ID			3
#define SECURITY_SERVER_PK_ID			4
#define SECURITY_SECRET_KEY_ID			5
#define SECURITY_SMS_MODE_ID			6
#define SECURITY_SMS_BINDING_KEY_PARAM_ID	7
#define SECURITY_SMS_BINDING_SECRET_KEY_ID	8
#define SECURITY_LWM2M_SERVER_SMS_NUM_ID	9
#define SECURITY_SHORT_SERVER_ID		10
#define SECURITY_CLIENT_HOLD_OFF_TIME_ID	11
#define SECURITY_BS_SERVER_ACCOUNT_TIMEOUT_ID	12
#if defined(CONFIG_LWM2M_SECURITY_OBJECT_VERSION_1_1)
#define SECURITY_MATCHING_TYPE_ID		13
#define SECURITY_SNI_ID				14
#define SECURITY_CERTIFICATE_USAGE_ID		15
#define SECURITY_DTLS_TLS_CIPHERSUITE_ID	16
#define SECURITY_OSCORE_SEC_MODE_ID		17
#endif

#define MAX_INSTANCE_COUNT		CONFIG_LWM2M_SECURITY_INSTANCE_COUNT

#define SECURITY_URI_LEN		255
#define IDENTITY_LEN			CONFIG_LWM2M_SECURITY_KEY_SIZE
#define KEY_LEN				CONFIG_LWM2M_SECURITY_KEY_SIZE

/*
 * Calculate resource instances as follows:
 * start with SECURITY_MAX_ID
 *
 * If using object version 1.1
 * subtract MULTI resources because their counts include 0 resource (1)
 * add DTLS_TLS_CIPHERSUITE_MAX for DTLS_TLS_CIPHERSUITE resource instances
 */
#if defined(CONFIG_LWM2M_SECURITY_OBJECT_VERSION_1_1)
#define RESOURCE_INSTANCE_COUNT (SECURITY_MAX_ID - 1 + DTLS_TLS_CIPHERSUITE_MAX)
#else
#define RESOURCE_INSTANCE_COUNT	(SECURITY_MAX_ID)
#endif

/* resource state variables */
static char  security_uri[MAX_INSTANCE_COUNT][SECURITY_URI_LEN];
static uint8_t  client_identity[MAX_INSTANCE_COUNT][IDENTITY_LEN];
static uint8_t  server_pk[MAX_INSTANCE_COUNT][KEY_LEN];
static uint8_t  secret_key[MAX_INSTANCE_COUNT][KEY_LEN];
static bool  bootstrap_flag[MAX_INSTANCE_COUNT];
static uint8_t  security_mode[MAX_INSTANCE_COUNT];
static uint16_t short_server_id[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj security;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(SECURITY_SERVER_URI_ID, RW, STRING),
	OBJ_FIELD_DATA(SECURITY_BOOTSTRAP_FLAG_ID, W, BOOL),
	OBJ_FIELD_DATA(SECURITY_MODE_ID, W, U8),
	OBJ_FIELD_DATA(SECURITY_CLIENT_PK_ID, W, OPAQUE),
	OBJ_FIELD_DATA(SECURITY_SERVER_PK_ID, W, OPAQUE),
	OBJ_FIELD_DATA(SECURITY_SECRET_KEY_ID, W, OPAQUE),
	OBJ_FIELD_DATA(SECURITY_SMS_MODE_ID, W_OPT, U8),
	OBJ_FIELD_DATA(SECURITY_SMS_BINDING_KEY_PARAM_ID, W_OPT, OPAQUE),
	OBJ_FIELD_DATA(SECURITY_SMS_BINDING_SECRET_KEY_ID, W_OPT, OPAQUE),
	OBJ_FIELD_DATA(SECURITY_LWM2M_SERVER_SMS_NUM_ID, W_OPT, STRING),
	OBJ_FIELD_DATA(SECURITY_SHORT_SERVER_ID, W_OPT, U16),
	OBJ_FIELD_DATA(SECURITY_CLIENT_HOLD_OFF_TIME_ID, W_OPT, S32),
	OBJ_FIELD_DATA(SECURITY_BS_SERVER_ACCOUNT_TIMEOUT_ID, W_OPT, S32),
#if defined(CONFIG_LWM2M_SECURITY_OBJECT_VERSION_1_1)
	OBJ_FIELD_DATA(SECURITY_MATCHING_TYPE_ID, W_OPT, S32),
	OBJ_FIELD_DATA(SECURITY_SNI_ID, W_OPT, STRING),
	OBJ_FIELD_DATA(SECURITY_CERTIFICATE_USAGE_ID, W_OPT, U32),
	OBJ_FIELD_DATA(SECURITY_DTLS_TLS_CIPHERSUITE_ID, W_OPT, U32),
	OBJ_FIELD_DATA(SECURITY_OSCORE_SEC_MODE_ID, W_OPT, OBJLNK)
#endif
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][SECURITY_MAX_ID];
static struct lwm2m_engine_res_inst
			res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj_inst *security_create(uint16_t obj_inst_id)
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

	/* default values */
	security_uri[index][0] = '\0';
	client_identity[index][0] = '\0';
	bootstrap_flag[index] = 0;
	security_mode[index] = 0U;
	short_server_id[index] = 0U;

	(void)memset(res[index], 0,
		     sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA_LEN(SECURITY_SERVER_URI_ID, res[index], i,
			  res_inst[index], j,
			  security_uri[index], SECURITY_URI_LEN, 0);
	INIT_OBJ_RES_DATA(SECURITY_BOOTSTRAP_FLAG_ID, res[index], i,
			  res_inst[index], j,
			  &bootstrap_flag[index], sizeof(*bootstrap_flag));
	INIT_OBJ_RES_DATA(SECURITY_MODE_ID, res[index], i,
			  res_inst[index], j,
			  &security_mode[index], sizeof(*security_mode));
	INIT_OBJ_RES_DATA_LEN(SECURITY_CLIENT_PK_ID, res[index], i,
			  res_inst[index], j,
			  &client_identity[index], IDENTITY_LEN, 0);
	INIT_OBJ_RES_DATA_LEN(SECURITY_SERVER_PK_ID, res[index], i,
			  res_inst[index], j,
			  &server_pk[index], KEY_LEN, 0);
	INIT_OBJ_RES_DATA_LEN(SECURITY_SECRET_KEY_ID, res[index], i,
			  res_inst[index], j,
			  &secret_key[index], KEY_LEN, 0);
	INIT_OBJ_RES_DATA(SECURITY_SHORT_SERVER_ID, res[index], i,
			  res_inst[index], j,
			  &short_server_id[index], sizeof(*short_server_id));
#if defined(CONFIG_LWM2M_SECURITY_OBJECT_VERSION_1_1)
	INIT_OBJ_RES_OPTDATA(SECURITY_MATCHING_TYPE_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(SECURITY_SNI_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(SECURITY_CERTIFICATE_USAGE_ID, res[index], i, res_inst[index], j);
	INIT_OBJ_RES_MULTI_OPTDATA(SECURITY_DTLS_TLS_CIPHERSUITE_ID, res[index], i,
				   res_inst[index], j, DTLS_TLS_CIPHERSUITE_MAX, false);
	INIT_OBJ_RES_OPTDATA(SECURITY_OSCORE_SEC_MODE_ID, res[index], i, res_inst[index], j);
#endif

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	LOG_DBG("Create LWM2M security instance: %d", obj_inst_id);

	return &inst[index];
}

int lwm2m_security_inst_id_to_index(uint16_t obj_inst_id)
{
	int i;

	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			return i;
		}
	}

	return -ENOENT;
}

int lwm2m_security_index_to_inst_id(int index)
{
	if (index >= MAX_INSTANCE_COUNT) {
		return -EINVAL;
	}

	/* not instantiated */
	if (!inst[index].obj) {
		return -ENOENT;
	}

	return inst[index].obj_inst_id;
}

int lwm2m_security_short_id_to_inst(uint16_t short_id)
{
	for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (short_server_id[i] == short_id) {
			return inst[i].obj_inst_id;
		}
	}
	return -ENOENT;
}

int lwm2m_security_mode(struct lwm2m_ctx *ctx)
{
	int ret;
	uint8_t mode;
	struct lwm2m_obj_path path =
		LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, ctx->sec_obj_inst, SECURITY_MODE_ID);

	ret = lwm2m_get_u8(&path, &mode);
	if (ret) {
		return ret;
	}
	return (int)mode;
}


static int lwm2m_security_init(void)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	security.obj_id = LWM2M_OBJECT_SECURITY_ID;
	security.version_major = SECURITY_VERSION_MAJOR;
	security.version_minor = SECURITY_VERSION_MINOR;
	security.is_core = true;
	security.fields = fields;
	security.field_count = ARRAY_SIZE(fields);
	security.max_instance_count = MAX_INSTANCE_COUNT;
	security.create_cb = security_create;
	lwm2m_register_obj(&security);

	/* auto create the first instance */
	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_SECURITY_ID, 0, &obj_inst);
	if (ret < 0) {
		LOG_ERR("Create LWM2M security instance 0 error: %d", ret);
	}

	return ret;
}

LWM2M_CORE_INIT(lwm2m_security_init);
