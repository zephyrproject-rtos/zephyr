/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_ac_control
#define LOG_LEVEL	CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "lwm2m_obj_access_control.h"

#include <stdint.h>

#include <zephyr/init.h>

#define READ		BIT(0)
#define WRITE		BIT(1)
#define ACEXEC		BIT(2)
#define DELETE		BIT(3)
#define CREATE		BIT(4)

/* For compatibility with lwm2m_op_perms */
#define WRITE_ATTR	BIT(8)
#define DISCOVER	BIT(9)

static int operation_to_acperm(int operation)
{
	switch (operation) {
	case LWM2M_OP_READ:
		return READ;

	case LWM2M_OP_WRITE:
		return WRITE;

	case LWM2M_OP_EXECUTE:
		return ACEXEC;

	case LWM2M_OP_DELETE:
		return DELETE;

	case LWM2M_OP_CREATE:
		return CREATE;

	case LWM2M_OP_WRITE_ATTR:
		return WRITE_ATTR;

	case LWM2M_OP_DISCOVER:
		return DISCOVER;
	default:
		return 0;
	}
}

#define ACCESS_CONTROL_VERSION_MAJOR 1
#define ACCESS_CONTROL_VERSION_MINOR 0
#define AC_OBJ_ID			LWM2M_OBJECT_ACCESS_CONTROL_ID
#define MAX_SERVER_COUNT	CONFIG_LWM2M_SERVER_INSTANCE_COUNT
#define MAX_INSTANCE_COUNT	CONFIG_LWM2M_ACCESS_CONTROL_INSTANCE_COUNT
#define OBJ_LVL_MAX_ID		65535

#define ACCESS_CONTROL_OBJECT_ID			0
#define ACCESS_CONTROL_OBJECT_INSTANCE_ID	1
#define ACCESS_CONTROL_ACL_ID				2
#define ACCESS_CONTROL_ACCESS_CONTROL_OWNER	3
#define ACCESS_CONTROL_MAX_ID				4


struct ac_data {
	uint16_t obj_id;
	uint16_t obj_inst_id;
	int16_t acl[MAX_SERVER_COUNT + 1];
	uint16_t ac_owner;
};

static struct ac_data ac_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj ac_obj;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(ACCESS_CONTROL_OBJECT_ID, RW, U16),
	OBJ_FIELD_DATA(ACCESS_CONTROL_OBJECT_INSTANCE_ID, RW, U16),
	/* Mark obj id and obj_inst id is RO, but needs to be written to by bootstrap server */
	OBJ_FIELD_DATA(ACCESS_CONTROL_ACL_ID, RW_OPT, U16),
	OBJ_FIELD_DATA(ACCESS_CONTROL_ACCESS_CONTROL_OWNER, RW, U16),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][ACCESS_CONTROL_MAX_ID];
/* Calculated as follows:
 * + ACCESS_CONTROL_MAX_ID - 1 (not counting the acl instance)
 * + MAX_SERVER_COUNT + 1 (one acl for every server plus default)
 */
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT]
					    [MAX_SERVER_COUNT + ACCESS_CONTROL_MAX_ID];

static int obj_inst_to_index(uint16_t obj_id, uint16_t obj_inst_id)
{
	int i, ret = -ENOENT;

	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		if (inst[i].obj && ac_data[i].obj_id == obj_id &&
		    ac_data[i].obj_inst_id == obj_inst_id) {
			ret = i;
			break;
		}
	}
	return ret;
}

static bool available_obj_inst_id(int obj_inst_id)
{
	for (int index = 0; index < ARRAY_SIZE(inst); index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			return false;
		}
	}
	return true;
}

void access_control_add(uint16_t obj_id, uint16_t obj_inst_id, int server_obj_inst_id)
{
	/* If ac_obj not created */
	if (!ac_obj.fields) {
		return;
	}

	if (obj_id == AC_OBJ_ID) {
		return;
	}

	if (obj_inst_to_index(obj_id, obj_inst_id) >= 0) {
		LOG_DBG("Access control for obj_inst /%d/%d already exist", obj_id, obj_inst_id);
		return;
	}

	int index, avail = -1;

	for (index = 0; index < ARRAY_SIZE(inst); index++) {
		/* Save first available slot index */
		if (avail < 0 && !inst[index].obj) {
			avail = index;
		}
	}

	if (avail < 0) {
		LOG_ERR("Can not create access control instance - no more room: %u", obj_inst_id);
		return;
	}

	int ssid;

	if (server_obj_inst_id < 0) {
		ssid = CONFIG_LWM2M_SERVER_DEFAULT_SSID;
	} else {
		ssid = lwm2m_server_get_ssid(server_obj_inst_id);
	}

	if (ssid < 0) {
		LOG_DBG("No server object instance %d - using default", server_obj_inst_id);
		ssid = CONFIG_LWM2M_SERVER_DEFAULT_SSID;
	}

	int ac_obj_inst_id = avail;

	while (!available_obj_inst_id(ac_obj_inst_id)) {
		ac_obj_inst_id++;
	}
	struct lwm2m_engine_obj_inst *obj_inst = NULL;

	lwm2m_create_obj_inst(AC_OBJ_ID, ac_obj_inst_id, &obj_inst);
	ac_data[avail].obj_id = obj_id;
	ac_data[avail].obj_inst_id = obj_inst_id;
	ac_data[avail].ac_owner = ssid;
}

void access_control_add_obj(uint16_t obj_id, int server_obj_inst_id)
{
	access_control_add(obj_id, OBJ_LVL_MAX_ID, server_obj_inst_id);
}

void access_control_remove(uint16_t obj_id, uint16_t obj_inst_id)
{
	/* If ac_obj not created */
	if (!ac_obj.fields) {
		return;
	}

	if (obj_id == AC_OBJ_ID) {
		return;
	}

	int idx = obj_inst_to_index(obj_id, obj_inst_id);

	if (idx < 0) {
		LOG_DBG("Cannot remove access control for /%d/%d - not found", obj_id, obj_inst_id);
		return;
	}

	ac_data[idx].obj_id = 0;
	ac_data[idx].obj_inst_id = 0;
	ac_data[idx].ac_owner = 0;
	for (int i = 0; i < MAX_SERVER_COUNT + 1; i++) {
		ac_data[idx].acl[i] = 0;
	}
	lwm2m_delete_obj_inst(AC_OBJ_ID, idx);
}

void access_control_remove_obj(uint16_t obj_id)
{
	access_control_remove(obj_id, OBJ_LVL_MAX_ID);
}

static bool check_acl_table(uint16_t obj_id, uint16_t obj_inst_id, uint16_t short_server_id,
			    uint16_t access)
{
	/* Get the index of the ac instance regarding obj_id and obj_inst_id */
	int idx = obj_inst_to_index(obj_id, obj_inst_id);

	if (idx < 0) {
		LOG_DBG("Access control for obj_inst /%d/%d not found", obj_id, obj_inst_id);
		return false;
	}

	uint16_t access_rights = 0;
	uint16_t default_rights = 0;
	bool server_has_acl = false;

	for (int i = 0; i < MAX_SERVER_COUNT + 1; i++) {
		int res_inst_id = res_inst[idx][ACCESS_CONTROL_ACL_ID + i].res_inst_id;
		/* If server has access or if default exist */
		if (res_inst_id == short_server_id) {
			access_rights |= ac_data[idx].acl[i];
			server_has_acl = true;
		} else if (res_inst_id == 0) {
			default_rights |= ac_data[idx].acl[i];
		}
	}

	if (server_has_acl) {
		return (access_rights & access) == access;
	}

	/* Full access if server is the ac_owner and no acl is specified for that server */
	if (ac_data[idx].ac_owner == short_server_id) {
		return true;
	}

	/* Return default rights */
	return (default_rights & access) == access;
}

int access_control_check_access(uint16_t obj_id, uint16_t obj_inst_id, uint16_t server_obj_inst,
				uint16_t operation, bool bootstrap_mode)
{
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	if (bootstrap_mode) {
		return 0; /* Full access for bootstrap servers */
	}
#else
	ARG_UNUSED(bootstrap_mode);
#endif
	/* If ac_obj not created */
	if (!ac_obj.fields) {
		return 0;
	}
	uint16_t access = operation_to_acperm(operation);
	int short_server_id = lwm2m_server_get_ssid(server_obj_inst);

	if (short_server_id < 0) {
		LOG_ERR("No server obj instance %u exist", server_obj_inst);
		return -EACCES;
	}

	if (obj_id == AC_OBJ_ID) {
		switch (access) {
		case READ:
			return 0;
		case ACEXEC:
		case DELETE:
		case CREATE:
			return -EPERM; /* Method not allowed */
		case WRITE:	       /* Only ac_owner can write to ac_obj */
			for (int index = 0; index < ARRAY_SIZE(inst); index++) {
				if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
					if (ac_data[index].ac_owner == short_server_id) {
						return 0;
					}
				}
			}
			return -EACCES;

		default:
			return -EACCES;
		}
	}

	/* only DISCOVER, WRITE_ATTR and CREATE allowed on object */
	if (obj_inst_id == OBJ_LVL_MAX_ID) {
		if (access == DISCOVER || access == WRITE_ATTR) {
			return 0;
		}

		if (access != CREATE) {
			return -EACCES;
		}
	}

	if (access == CREATE) {
		obj_inst_id = OBJ_LVL_MAX_ID;
	}

	if (check_acl_table(obj_id, obj_inst_id, short_server_id, access)) {
		return 0;
	}

	return -EACCES;
}

static void add_existing_objects(void)
{
	/* register all objects in the sys-list */
	struct lwm2m_engine_obj *obj;

	SYS_SLIST_FOR_EACH_CONTAINER(lwm2m_engine_obj_list(), obj, node) {
		access_control_add_obj(obj->obj_id, -1);
	}

	/* register all object instances in the sys-list */
	struct lwm2m_engine_obj_inst *obj_inst;

	SYS_SLIST_FOR_EACH_CONTAINER(lwm2m_engine_obj_inst_list(), obj_inst, node) {
		access_control_add(obj_inst->obj->obj_id, obj_inst->obj_inst_id, -1);
	}
}

static int write_validate_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			     uint8_t *data, uint16_t data_len, bool last_block, size_t total_size)
{
	/* validates and removes acl instances for non-existing servers */

	if (res_inst_id == 0) {
		return 0;
	}

	/* If there is a server instance with ssid == res_inst_id, return */
	if (lwm2m_server_short_id_to_inst(res_inst_id) >= 0) {
		return 0;
	}

	/* if that res inst id does not match any ssid's, remove it */
	int idx = -1;

	for (int i = 0; i < ARRAY_SIZE(inst); i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			idx = i;
			break;
		}
	}

	if (idx < 0) {
		LOG_ERR("Object instance not found - %u", obj_inst_id);
		return -ENOENT;
	}

	for (int i = 0; i < ARRAY_SIZE(inst); i++) {
		if (res_inst[idx][ACCESS_CONTROL_ACL_ID + i].res_inst_id == res_inst_id) {
			res_inst[idx][ACCESS_CONTROL_ACL_ID + i].res_inst_id =
				RES_INSTANCE_NOT_CREATED;
			break;
		}
	}
	return 0;
}

static struct lwm2m_engine_obj_inst *ac_create(uint16_t obj_inst_id)
{
	int index, avail = -1, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < ARRAY_SIZE(inst); index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Can not create access control instance - "
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
		LOG_ERR("Can not create access control instance - no more room: %u", obj_inst_id);
		return NULL;
	}

	/* Set default values */
	(void)memset(res[avail], 0, sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));
	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(ACCESS_CONTROL_OBJECT_ID, res[avail], i, res_inst[avail], j,
			  &ac_data[avail].obj_id, sizeof(ac_data[avail].obj_id));
	INIT_OBJ_RES_DATA(ACCESS_CONTROL_OBJECT_INSTANCE_ID, res[avail], i, res_inst[avail], j,
			  &ac_data[avail].obj_inst_id, sizeof(ac_data[avail].obj_inst_id));
	INIT_OBJ_RES(ACCESS_CONTROL_ACL_ID, res[avail], i, res_inst[avail], j, MAX_SERVER_COUNT + 1,
		     true, false, ac_data[avail].acl, sizeof(ac_data[avail].acl[0]), NULL, NULL,
		     write_validate_cb, NULL, NULL);
	INIT_OBJ_RES_DATA(ACCESS_CONTROL_ACCESS_CONTROL_OWNER, res[avail], i, res_inst[avail], j,
			  &ac_data[avail].ac_owner, sizeof(ac_data[avail].ac_owner));

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create access control instance: %d", obj_inst_id);
	return &inst[avail];
}

static int ac_control_init(void)
{
	ac_obj.obj_id = LWM2M_OBJECT_ACCESS_CONTROL_ID;
	ac_obj.version_major = ACCESS_CONTROL_VERSION_MAJOR;
	ac_obj.version_minor = ACCESS_CONTROL_VERSION_MINOR;
	ac_obj.is_core = true;
	ac_obj.fields = fields;
	ac_obj.field_count = ARRAY_SIZE(fields);
	ac_obj.max_instance_count = ARRAY_SIZE(inst);
	ac_obj.create_cb = ac_create;
	lwm2m_register_obj(&ac_obj);

	if (!IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)) {
		/* add the objects/object instances that were created before the ac control */
		add_existing_objects();
	}
	return 0;
}

SYS_INIT(ac_control_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
