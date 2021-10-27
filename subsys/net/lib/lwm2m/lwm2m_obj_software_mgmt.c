/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_obj_software_mgmt
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define SOFTWARE_MGMT_VERSION_MAJOR 1
#define SOFTWARE_MGMT_VERSION_MINOR 0

/* Software Management resource IDs */
#define SOFTWARE_MGMT_PACKAGE_NAME_ID                   0
#define SOFTWARE_MGMT_PACKAGE_VERSION_ID                1
#define SOFTWARE_MGMT_PACKAGE_ID                        2
#define SOFTWARE_MGMT_PACKAGE_URI_ID                    3
#define SOFTWARE_MGMT_INSTALL_ID                        4
#define SOFTWARE_MGMT_CHECKPOINT_ID                     5
#define SOFTWARE_MGMT_UNINSTALL_ID                      6
#define SOFTWARE_MGMT_UPDATE_STATE_ID                   7
#define SOFTWARE_MGMT_UPDATE_SUPPORTED_OBJECTS_ID       8
#define SOFTWARE_MGMT_UPDATE_RESULT_ID                  9
#define SOFTWARE_MGMT_ACTIVATE_ID                       10
#define SOFTWARE_MGMT_DEACTIVATE_ID                     11
#define SOFTWARE_MGMT_ACTIVATION_STATE_ID               12
#define SOFTWARE_MGMT_PACKAGE_SETTINGS_ID               13
#define SOFTWARE_MGMT_USER_NAME_ID                      14
#define SOFTWARE_MGMT_PASSWORD_ID                       15
#define SOFTWARE_MGMT_STATUS_REASON_ID                  16
#define SOFTWARE_MGMT_SOFTWARE_COMPONENT_LINK_ID        17
#define SOFTWARE_MGMT_SOFTWARE_COMPONENT_TREE_LENGTH_ID 18

#define SOFTWARE_MGMT_MAX_ID            19

#define DELIVERY_METHOD_PULL_ONLY       0
#define DELIVERY_METHOD_PUSH_ONLY       1
#define DELIVERY_METHOD_BOTH            2

#define PACKAGE_URI_LEN				    255
#define PACKAGE_NAME_LEN                255
#define PACKAGE_VERSION_LEN             255

#define MAX_INSTANCE_COUNT		CONFIG_LWM2M_SOFTWARE_MANAGEMENT_INSTANCE_COUNT

#define RESOURCE_INSTANCE_COUNT     (SOFTWARE_MGMT_MAX_ID - 4)

/* resource state variables */
static uint8_t update_state[MAX_INSTANCE_COUNT];
static uint8_t update_result[MAX_INSTANCE_COUNT];
static uint8_t activation_state[MAX_INSTANCE_COUNT];
static char package_uri[MAX_INSTANCE_COUNT][PACKAGE_URI_LEN];
static char package_name[MAX_INSTANCE_COUNT][PACKAGE_NAME_LEN];
static char package_version[MAX_INSTANCE_COUNT][PACKAGE_VERSION_LEN];

static lwm2m_engine_set_data_cb_t write_cb;
static lwm2m_engine_execute_cb_t install_cb;
static lwm2m_engine_execute_cb_t uninstall_cb;
static lwm2m_engine_execute_cb_t activate_cb;
static lwm2m_engine_execute_cb_t deactivate_cb;

static struct lwm2m_engine_obj software_mgmt;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(SOFTWARE_MGMT_PACKAGE_NAME_ID, R, STRING),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_PACKAGE_VERSION_ID, R, STRING),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_PACKAGE_ID, W_OPT, OPAQUE),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_PACKAGE_URI_ID, W_OPT, STRING),
	OBJ_FIELD_EXECUTE(SOFTWARE_MGMT_INSTALL_ID),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_CHECKPOINT_ID, R_OPT, OBJLNK),
	OBJ_FIELD_EXECUTE(SOFTWARE_MGMT_UNINSTALL_ID),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_UPDATE_STATE_ID, R, U8),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_UPDATE_SUPPORTED_OBJECTS_ID, RW_OPT, BOOL),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_UPDATE_RESULT_ID, R, U8),
	OBJ_FIELD_EXECUTE(SOFTWARE_MGMT_ACTIVATE_ID),
	OBJ_FIELD_EXECUTE(SOFTWARE_MGMT_DEACTIVATE_ID),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_ACTIVATION_STATE_ID, R, BOOL),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_PACKAGE_SETTINGS_ID, RW_OPT, OBJLNK),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_USER_NAME_ID, W_OPT, STRING),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_PASSWORD_ID, W_OPT, STRING),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_STATUS_REASON_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_SOFTWARE_COMPONENT_LINK_ID, R_OPT, OBJLNK),
	OBJ_FIELD_DATA(SOFTWARE_MGMT_SOFTWARE_COMPONENT_TREE_LENGTH_ID, R_OPT, U8)
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][SOFTWARE_MGMT_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static uint8_t lwm2m_software_mgmt_instance_id_to_index(uint16_t obj_inst_id)
{
	int i;

	for (i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (inst[i].obj && inst[i].obj_inst_id == obj_inst_id) {
			return i;
		}
	}

	LOG_ERR("No instance found with id %d", obj_inst_id);
	return -ENOENT;
}

void lwm2m_software_mgmt_set_package_name(uint16_t obj_inst_id, const char *name)
{
	int i;

	LOG_DBG("Set package name for %d", obj_inst_id);
	i = lwm2m_software_mgmt_instance_id_to_index(obj_inst_id);

	if (i < 0) {
		return;
	}

	strncpy(package_name[i], name, sizeof(package_name[i]));
}

void lwm2m_software_mgmt_set_package_version(uint16_t obj_inst_id, const char *version)
{
	int i;

	LOG_DBG("Set package version for %d", obj_inst_id);
	i = lwm2m_software_mgmt_instance_id_to_index(obj_inst_id);

	if (i < 0) {
		return;
	}

	strncpy(package_version[i], version, sizeof(package_version[i]));
}

uint8_t lwm2m_software_mgmt_get_update_state(uint16_t obj_inst_id)
{
	int i;

	LOG_DBG("Get update state for %d", obj_inst_id);
	i = lwm2m_software_mgmt_instance_id_to_index(obj_inst_id);

	if (i < 0) {
		return -ENOENT;
	}

	return update_state[i];
}

void lwm2m_software_mgmt_set_update_state(uint16_t obj_inst_id, uint8_t state)
{
	int i;
	bool error_transition = false;

	LOG_DBG("Set update state to %d for %d", state, obj_inst_id);
	i = lwm2m_software_mgmt_instance_id_to_index(obj_inst_id);

	if (i < 0) {
		return;
	}

	switch (state) {
	case SW_MGMT_UPDATE_STATE_INITIAL:
		break;
	case SW_MGMT_UPDATE_STATE_DOWNLOAD_STARTED:
		if (update_state[i] != SW_MGMT_UPDATE_STATE_INITIAL) {
			error_transition = true;
		}
		break;
	case SW_MGMT_UPDATE_STATE_DOWNLOADED:
		if (update_state[i] != SW_MGMT_UPDATE_STATE_DOWNLOAD_STARTED) {
			error_transition = true;
		}
		break;
	case SW_MGMT_UPDATE_STATE_DELIVERED:
		if (update_state[i] != SW_MGMT_UPDATE_STATE_DOWNLOADED) {
			error_transition = true;
		}
		break;
	case SW_MGMT_UPDATE_STATE_INSTALLED:
		if (update_state[i] != SW_MGMT_UPDATE_STATE_DELIVERED) {
			error_transition = true;
		}
		break;
	default:
		LOG_ERR("Unknown state %d", state);
		return;
	}

	if (error_transition) {
		LOG_ERR("Unsupported state transition from %d to %d", update_state[i], state);

	} else {
		if (update_state[i] == SW_MGMT_UPDATE_STATE_INSTALLED && update_state[i] != state) {
			activation_state[i] = SW_MGMT_ACTIVATION_STATE_DISABLED;
		}
		update_state[i] = state;
	}
}

void lwm2m_software_mgmt_set_update_result(uint16_t obj_inst_id, uint8_t result)
{
	int i;

	LOG_DBG("Set update state to %d for %d", result, obj_inst_id);
	i = lwm2m_software_mgmt_instance_id_to_index(obj_inst_id);

	if (i < 0) {
		return;
	}

	switch (result) {
	case SW_MGMT_UPDATE_RESULT_DEFAULT:
		lwm2m_software_mgmt_set_update_state(obj_inst_id, SW_MGMT_UPDATE_STATE_INITIAL);
		break;
	case SW_MGMT_UPDATE_RESULT_DOWNLOADING:
		lwm2m_software_mgmt_set_update_state(obj_inst_id,
						     SW_MGMT_UPDATE_STATE_DOWNLOAD_STARTED);
		break;
	case SW_MGMT_UPDATE_RESULT_INSTALLED:
		lwm2m_software_mgmt_set_update_state(obj_inst_id, SW_MGMT_UPDATE_STATE_INSTALLED);
		break;
	case SW_MGMT_UPDATE_RESULT_DOWNLOADED_VERIFIED:
		lwm2m_software_mgmt_set_update_state(obj_inst_id, SW_MGMT_UPDATE_STATE_DELIVERED);
		break;
	case SW_MGMT_UPDATE_RESULT_OUT_OF_STORAGE:
	case SW_MGMT_UPDATE_RESULT_OUT_OF_MEM:
	case SW_MGMT_UPDATE_RESULT_CONNECTION_LOST:
	case SW_MGMT_UPDATE_RESULT_INTEGRITY_CHECK_FAILED:
	case SW_MGMT_UPDATE_RESULT_UNSUP_PACKAGE_TYPE:
	case SW_MGMT_UPDATE_RESULT_INVALID_URI:
	case SW_MGMT_UPDATE_RESULT_UPDATE_ERROR:
		lwm2m_software_mgmt_set_update_state(obj_inst_id, SW_MGMT_UPDATE_STATE_INITIAL);
		break;
	case SW_MGMT_UPDATE_RESULT_INSTALLATION_FAILURE:
		break;
	case SW_MGMT_UPDATE_RESULT_UNINSTALLATION_FAILURE_FOR_UPDATE:
		break;
	default:
		LOG_ERR("Unknown result %d", result);
		return;
	}

	update_result[i] = result;
}

uint8_t lwm2m_software_mgmt_get_update_result(uint16_t obj_inst_id)
{
	int i;

	LOG_DBG("Get update result from %d", obj_inst_id);
	i = lwm2m_software_mgmt_instance_id_to_index(obj_inst_id);

	if (i < 0) {
		return -ENOENT;
	}

	return update_result[i];
}

uint8_t lwm2m_software_mgmt_get_activation_state(uint16_t obj_inst_id)
{
	int i;

	LOG_DBG("Set activation state for %d", obj_inst_id);
	i = lwm2m_software_mgmt_instance_id_to_index(obj_inst_id);

	if (i < 0) {
		return -ENOENT;
	}

	return activation_state[i];
}

void lwm2m_software_mgmt_set_activation_state(uint16_t obj_inst_id, uint8_t state)
{
	int i;

	LOG_DBG("Set update state to %d for %d", state, obj_inst_id);
	i = lwm2m_software_mgmt_instance_id_to_index(obj_inst_id);

	if (i < 0) {
		return;
	}

	/* Only allowed to change the state when in installed */
	if (update_state[i] == SW_MGMT_UPDATE_STATE_INSTALLED) {
		activation_state[i] = state;
	} else {
		LOG_ERR("Activation state machine inactive outside installed state");
	}
}

static int package_write_cb(uint16_t obj_inst_id, uint16_t res_id,
			    uint16_t res_inst_id, uint8_t *data, uint16_t data_len,
			    bool last_block, size_t total_size)
{
	LOG_DBG("Package write callback for %d", obj_inst_id);
	uint8_t state;
	int ret;

	state = lwm2m_software_mgmt_get_update_state(obj_inst_id);
	if (state == SW_MGMT_UPDATE_STATE_INITIAL) {
		lwm2m_software_mgmt_set_update_state(obj_inst_id,
						     SW_MGMT_UPDATE_STATE_DOWNLOAD_STARTED);
	} else if (state != SW_MGMT_UPDATE_STATE_DOWNLOAD_STARTED) {
		if (data_len == 0U && state == SW_MGMT_UPDATE_STATE_DOWNLOADED) {
			/* reset to state idle and result default */
			lwm2m_software_mgmt_set_update_result(obj_inst_id,
							      SW_MGMT_UPDATE_RESULT_DEFAULT);
			return 0;
		}

		LOG_DBG("Cannot download: state = %d", state);
		return -EPERM;
	}

	ret = write_cb ? write_cb(obj_inst_id, res_id, res_inst_id,
				  data, data_len,
				  last_block, total_size) : 0;
	if (ret >= 0) {
		if (last_block) {
			lwm2m_software_mgmt_set_update_state(obj_inst_id,
							     SW_MGMT_UPDATE_STATE_DOWNLOADED);
			lwm2m_software_mgmt_set_update_state(obj_inst_id,
							     SW_MGMT_UPDATE_STATE_DELIVERED);
		}

		return 0;
	} else if (ret == -ENOMEM) {
		lwm2m_software_mgmt_set_update_result(obj_inst_id,
						      SW_MGMT_UPDATE_RESULT_OUT_OF_MEM);
	} else if (ret == -ENOSPC) {
		lwm2m_software_mgmt_set_update_result(obj_inst_id,
						      SW_MGMT_UPDATE_RESULT_OUT_OF_STORAGE);
		/* Response 4.13 (RFC7959, section 2.9.3) */
		/* TODO: should include size1 option to indicate max size */
		ret = -EFBIG;
	} else if (ret == -EFAULT) {
		lwm2m_software_mgmt_set_update_result(obj_inst_id,
						      SW_MGMT_UPDATE_RESULT_INTEGRITY_CHECK_FAILED);
	} else {
		lwm2m_software_mgmt_set_update_result(obj_inst_id,
						      SW_MGMT_UPDATE_RESULT_UPDATE_ERROR);
	}

	return ret;
}

static int software_mgmt_install_cb(uint16_t obj_inst_id,
				    uint8_t *args, uint16_t args_len)
{
	lwm2m_engine_execute_cb_t callback;
	uint8_t state;
	int ret;

	state = lwm2m_software_mgmt_get_update_state(obj_inst_id);
	if (state != SW_MGMT_UPDATE_STATE_DELIVERED) {
		LOG_ERR("State other than downloaded: %d", state);
		return -EPERM;
	}

	callback = lwm2m_software_mgmt_get_install_cb();
	if (callback) {
		ret = callback(obj_inst_id, args, args_len);
		if (ret < 0) {
			LOG_ERR("Failed to install software: %d", ret);
			lwm2m_software_mgmt_set_update_result(obj_inst_id,
				ret == -EINVAL ? SW_MGMT_UPDATE_RESULT_INTEGRITY_CHECK_FAILED :
						 SW_MGMT_UPDATE_RESULT_UPDATE_ERROR);
			return ret;
		}
	}

	return 0;
}

static int software_mgmt_uninstall_cb(uint16_t obj_inst_id,
				      uint8_t *args, uint16_t args_len)
{
	lwm2m_engine_execute_cb_t callback;
	uint8_t state;
	int ret;

	state = lwm2m_software_mgmt_get_update_state(obj_inst_id);
	if (state != SW_MGMT_UPDATE_STATE_DELIVERED || state != SW_MGMT_UPDATE_STATE_INSTALLED) {
		LOG_ERR("State other than delivered or installed: %d", state);
		return -EPERM;
	}

	callback = lwm2m_software_mgmt_get_uninstall_cb();
	if (callback) {
		ret = callback(obj_inst_id, args, args_len);
		if (ret < 0) {
			LOG_ERR("Failed to uninstall software: %d", ret);
			return ret;
		}
		lwm2m_software_mgmt_set_update_state(obj_inst_id, SW_MGMT_UPDATE_STATE_INITIAL);
	}

	return 0;
}

static int software_mgmt_activate_cb(uint16_t obj_inst_id,
				     uint8_t *args, uint16_t args_len)
{
	lwm2m_engine_execute_cb_t callback;
	uint8_t state;
	int ret;

	state = lwm2m_software_mgmt_get_update_state(obj_inst_id);
	if (state != SW_MGMT_UPDATE_STATE_INSTALLED) {
		LOG_ERR("State other than installed: %d", state);
		return -EPERM;
	}

	callback = lwm2m_software_mgmt_get_activate_cb();
	if (callback) {
		ret = callback(obj_inst_id, args, args_len);
		if (ret < 0) {
			LOG_ERR("Failed to activate software: %d", ret);
			return ret;
		}
		lwm2m_software_mgmt_set_activation_state(obj_inst_id,
							 SW_MGMT_ACTIVATION_STATE_ENABLED);
	}

	return 0;
}

static int software_mgmt_deactivate_cb(uint16_t obj_inst_id,
				       uint8_t *args, uint16_t args_len)
{
	lwm2m_engine_execute_cb_t callback;
	uint8_t state;
	int ret;

	state = lwm2m_software_mgmt_get_update_state(obj_inst_id);
	if (state != SW_MGMT_UPDATE_STATE_INSTALLED) {
		LOG_ERR("State other than installed: %d", state);
		return -EPERM;
	}

	callback = lwm2m_software_mgmt_get_deactivate_cb();
	if (callback) {
		ret = callback(obj_inst_id, args, args_len);
		if (ret < 0) {
			LOG_ERR("Failed to deactivate software: %d", ret);
			return ret;
		}
		lwm2m_software_mgmt_set_activation_state(obj_inst_id,
							 SW_MGMT_ACTIVATION_STATE_DISABLED);
	}

	return 0;
}

static int package_uri_write_cb(uint16_t obj_inst_id, uint16_t res_id,
				uint16_t res_inst_id, uint8_t *data, uint16_t data_len,
				bool last_block, size_t total_size)
{
	LOG_ERR("Not supported");
	return -EINVAL;
}

void lwm2m_software_mgmt_set_write_cb(lwm2m_engine_set_data_cb_t cb)
{
	write_cb = cb;
}

lwm2m_engine_set_data_cb_t lwm2m_software_mgmt_get_write_cb(void)
{
	return write_cb;
}

void lwm2m_software_mgmt_set_install_cb(lwm2m_engine_execute_cb_t cb)
{
	install_cb = cb;
}

lwm2m_engine_execute_cb_t lwm2m_software_mgmt_get_install_cb(void)
{
	return install_cb;
}

void lwm2m_software_mgmt_set_uninstall_cb(lwm2m_engine_execute_cb_t cb)
{
	uninstall_cb = cb;
}

lwm2m_engine_execute_cb_t lwm2m_software_mgmt_get_uninstall_cb(void)
{
	return uninstall_cb;
}

void lwm2m_software_mgmt_set_activate_cb(lwm2m_engine_execute_cb_t cb)
{
	activate_cb = cb;
}

lwm2m_engine_execute_cb_t lwm2m_software_mgmt_get_activate_cb(void)
{
	return activate_cb;
}

void lwm2m_software_mgmt_set_deactivate_cb(lwm2m_engine_execute_cb_t cb)
{
	deactivate_cb = cb;
}

lwm2m_engine_execute_cb_t lwm2m_software_mgmt_get_deactivate_cb(void)
{
	return deactivate_cb;
}

static struct lwm2m_engine_obj_inst *software_mgmt_create(uint16_t obj_inst_id)
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
	update_state[index] = SW_MGMT_UPDATE_STATE_INITIAL;
	activation_state[index] = SW_MGMT_ACTIVATION_STATE_DISABLED;
	update_result[index] = SW_MGMT_UPDATE_RESULT_DEFAULT;
	package_uri[index][0] = '\0';
	package_name[index][0] = '\0';
	package_version[index][0] = '\0';

	(void)memset(res[index], 0,
			 sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA(SOFTWARE_MGMT_PACKAGE_NAME_ID, res[index], i,
			  res_inst[index], j,
			  &package_name[index], PACKAGE_NAME_LEN);
	INIT_OBJ_RES_DATA(SOFTWARE_MGMT_PACKAGE_VERSION_ID, res[index], i,
			  res_inst[index], j,
			  &package_version[index], PACKAGE_VERSION_LEN);
	INIT_OBJ_RES_OPT(SOFTWARE_MGMT_PACKAGE_ID, res[index], i, res_inst[index], j, 1, false,
			 true, NULL, NULL, NULL, package_write_cb, NULL);
	INIT_OBJ_RES(SOFTWARE_MGMT_PACKAGE_URI_ID, res[index], i, res_inst[index], j, 1, false,
			 true, package_uri[index], PACKAGE_URI_LEN,
			 NULL, NULL, NULL, package_uri_write_cb, NULL);
	INIT_OBJ_RES_EXECUTE(SOFTWARE_MGMT_INSTALL_ID, res[index], i, software_mgmt_install_cb);
	INIT_OBJ_RES_EXECUTE(SOFTWARE_MGMT_UNINSTALL_ID, res[index], i, software_mgmt_uninstall_cb);
	INIT_OBJ_RES_DATA(SOFTWARE_MGMT_UPDATE_STATE_ID, res[index], i, res_inst[index], j,
			  &update_state[index], sizeof(update_state[index]));
	INIT_OBJ_RES_DATA(SOFTWARE_MGMT_UPDATE_RESULT_ID, res[index], i, res_inst[index], j,
			  &update_result[index], sizeof(update_result[index]));
	INIT_OBJ_RES_EXECUTE(SOFTWARE_MGMT_ACTIVATE_ID, res[index], i, software_mgmt_activate_cb);
	INIT_OBJ_RES_EXECUTE(SOFTWARE_MGMT_DEACTIVATE_ID, res[index], i,
			     software_mgmt_deactivate_cb);
	INIT_OBJ_RES_DATA(SOFTWARE_MGMT_ACTIVATION_STATE_ID, res[index], i, res_inst[index], j,
			  &activation_state[index], sizeof(activation_state[index]));

	inst[index].resources = res[index];
	inst[index].resource_count = i;
	LOG_DBG("Create LWM2M Software Management instance: %d", obj_inst_id);
	return &inst[index];
}

static int lwm2m_software_mgmt_init(const struct device *dev)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	software_mgmt.obj_id = LWM2M_OBJECT_SOFTWARE_MANAGEMENT_ID;
	software_mgmt.version_major = SOFTWARE_MGMT_VERSION_MAJOR;
	software_mgmt.version_minor = SOFTWARE_MGMT_VERSION_MINOR;
	software_mgmt.is_core = true;
	software_mgmt.fields = fields;
	software_mgmt.field_count = ARRAY_SIZE(fields);
	software_mgmt.max_instance_count = MAX_INSTANCE_COUNT;
	software_mgmt.create_cb = software_mgmt_create;
	lwm2m_register_obj(&software_mgmt);

	/* auto create the first instance */
	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_SOFTWARE_MANAGEMENT_ID, 0, &obj_inst);
	if (ret < 0) {
		LOG_ERR("Create LWM2M server instance 0 error: %d", ret);
	}

	return ret;
}

SYS_INIT(lwm2m_software_mgmt_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
