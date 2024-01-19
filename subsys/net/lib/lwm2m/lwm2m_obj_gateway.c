/**
 * @file lwm2m_gateway.c
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Gateway
 * https://github.com/OpenMobileAlliance/lwm2m-registry/blob/prod/25.xml
 */

#define LOG_MODULE_NAME net_lwm2m_gateway
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"
#include "lwm2m_obj_gateway.h"

#define GATEWAY_VERSION_MAJOR 2
#define GATEWAY_VERSION_MINOR 0
#define GATEWAY_MAX_ID 4

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_GATEWAY_MAX_INSTANCES

BUILD_ASSERT(CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE >= CONFIG_LWM2M_GATEWAY_PREFIX_MAX_STR_SIZE,
	     "Gateway prefix requires validation");

/*
 * Calculate resource instances as follows:
 * start with GATEWAY_MAX_ID
 * subtract EXEC resources (1)
 */
#define RESOURCE_INSTANCE_COUNT (GATEWAY_MAX_ID - 1)

struct lwm2m_gw_obj {
	char device_id[CONFIG_LWM2M_GATEWAY_DEVICE_ID_MAX_STR_SIZE];
	char prefix[CONFIG_LWM2M_GATEWAY_PREFIX_MAX_STR_SIZE];
	char iot_device_objects[CONFIG_LWM2M_GATEWAY_IOT_DEVICE_OBJECTS_MAX_STR_SIZE];
};

static struct lwm2m_gw_obj device_table[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_obj lwm2m_gw;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(LWM2M_GATEWAY_DEVICE_RID, R, STRING),
	OBJ_FIELD_DATA(LWM2M_GATEWAY_PREFIX_RID, RW, STRING),
	OBJ_FIELD_DATA(LWM2M_GATEWAY_IOT_DEVICE_OBJECTS_RID, R, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][GATEWAY_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];
lwm2m_engine_gateway_msg_cb gateway_msg_cb[MAX_INSTANCE_COUNT];

static int prefix_validation_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				uint8_t *data, uint16_t data_len, bool last_block,
				size_t total_size)
{
	int i;
	int length;
	bool unique = true;

	/* Prefix can't be empty because it is used to reference device */
	if (data_len == 0) {
		return -EINVAL;
	}

	/* Prefix of each gateway must be unique */
	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		length = strlen(device_table[i].prefix);
		if (length == data_len) {
			if (strncmp(device_table[i].prefix, data, length) == 0) {
				if (inst[i].obj_inst_id != obj_inst_id) {
					unique = false;
					break;
				}
			}
		}
	}

	return unique ? 0 : -EINVAL;
}

static struct lwm2m_engine_obj_inst *lwm2m_gw_create(uint16_t obj_inst_id)
{
	int index, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Can not create instance - "
				"already existing: %u",
				obj_inst_id);
			return NULL;
		}
	}

	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (!inst[index].obj) {
			break;
		}
	}

	if (index >= MAX_INSTANCE_COUNT) {
		LOG_ERR("Can not create instance - no more room: %u", obj_inst_id);
		return NULL;
	}

	/* Set default values */
	strncpy(device_table[index].device_id, CONFIG_LWM2M_GATEWAY_DEFAULT_DEVICE_ID,
		CONFIG_LWM2M_GATEWAY_DEVICE_ID_MAX_STR_SIZE);
	snprintk(device_table[index].prefix, CONFIG_LWM2M_GATEWAY_PREFIX_MAX_STR_SIZE,
		 CONFIG_LWM2M_GATEWAY_DEFAULT_DEVICE_PREFIX "%u", index);
	strncpy(device_table[index].iot_device_objects,
		CONFIG_LWM2M_GATEWAY_DEFAULT_IOT_DEVICE_OBJECTS,
		CONFIG_LWM2M_GATEWAY_IOT_DEVICE_OBJECTS_MAX_STR_SIZE);

	(void)memset(res[index], 0, sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA_LEN(LWM2M_GATEWAY_DEVICE_RID, res[index], i, res_inst[index], j,
			      device_table[index].device_id,
			      CONFIG_LWM2M_GATEWAY_DEVICE_ID_MAX_STR_SIZE,
			      strlen(device_table[index].device_id) + 1);
	INIT_OBJ_RES_LEN(LWM2M_GATEWAY_PREFIX_RID, res[index], i, res_inst[index], j, 1, false,
			 true, device_table[index].prefix, CONFIG_LWM2M_GATEWAY_PREFIX_MAX_STR_SIZE,
			 strlen(device_table[index].prefix) + 1, NULL, NULL, prefix_validation_cb,
			 NULL, NULL);
	INIT_OBJ_RES_DATA_LEN(LWM2M_GATEWAY_IOT_DEVICE_OBJECTS_RID, res[index], i, res_inst[index],
			      j, device_table[index].iot_device_objects,
			      sizeof(device_table[index].iot_device_objects),
			      strlen(device_table[index].iot_device_objects) + 1);

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	LOG_DBG("Created LWM2M gateway instance: %d", obj_inst_id);
	return &inst[index];
}

int lwm2m_gw_handle_req(struct lwm2m_message *msg)
{
	struct coap_option options[4];
	int ret;

	ret = coap_find_options(msg->in.in_cpkt, COAP_OPTION_URI_PATH, options,
				ARRAY_SIZE(options));
	if (ret < 0) {
		return ret;
	}

	for (int index = 0; index < MAX_INSTANCE_COUNT; index++) {
		/* Skip uninitialized objects */
		if (!inst[index].obj) {
			continue;
		}

		char *prefix = device_table[index].prefix;
		size_t prefix_len = strlen(prefix);

		if (prefix_len != options[0].len) {
			continue;
		}
		if (strncmp(options[0].value, prefix, prefix_len) != 0) {
			continue;
		}

		if (gateway_msg_cb[index] == NULL) {
			return -ENOENT;
		}
		/* Delete prefix from path*/
		ret = coap_options_to_path(&options[1], ret - 1, &msg->path);
		if (ret < 0) {
			return ret;
		}
		return gateway_msg_cb[index](msg);
	}
	return -ENOENT;
}

int lwm2m_register_gw_callback(uint16_t obj_inst_id, lwm2m_engine_gateway_msg_cb cb)
{
	for (int index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj_inst_id == obj_inst_id) {
			gateway_msg_cb[index] = cb;
			return 0;
		}
	}
	return -ENOENT;
}

static int lwm2m_gw_init(void)
{
	int ret = 0;

	/* initialize the LwM2M Gateway field data */
	lwm2m_gw.obj_id = LWM2M_OBJECT_GATEWAY_ID;
	lwm2m_gw.version_major = GATEWAY_VERSION_MAJOR;
	lwm2m_gw.version_minor = GATEWAY_VERSION_MINOR;
	lwm2m_gw.is_core = false;
	lwm2m_gw.fields = fields;
	lwm2m_gw.field_count = ARRAY_SIZE(fields);
	lwm2m_gw.max_instance_count = MAX_INSTANCE_COUNT;
	lwm2m_gw.create_cb = lwm2m_gw_create;
	lwm2m_register_obj(&lwm2m_gw);
	return ret;
}

LWM2M_OBJ_INIT(lwm2m_gw_init);
