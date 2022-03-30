/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 * Copyright (c) 2021 Voi Technology AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_obj_swmgmt
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <zephyr/init.h>

#include <stdint.h>
#include <zephyr/net/lwm2m.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#include "lwm2m_pull_context.h"

#define SWMGMT_VERSION_MAJOR 1
#define SWMGMT_VERSION_MINOR 0

#define SWMGMT_PACKAGE_NAME_ID 0
#define SWMGMT_PACKAGE_VERSION_ID 1
#define SWMGMT_PACKAGE_ID 2
#define SWMGMT_PACKAGE_URI_ID 3
#define SWMGMT_INSTALL_ID 4
#define SWMGMT_CHECKPOINT_ID 5
#define SWMGMT_UNINSTALL_ID 6
#define SWMGMT_UPDATE_STATE_ID 7
#define SWMGMT_UPDATE_SUPPORTED_OBJECTS_ID 8
#define SWMGMT_UPDATE_RESULT_ID 9
#define SWMGMT_ACTIVATE_ID 10
#define SWMGMT_DEACTIVATE_ID 11
#define SWMGMT_ACTIVATION_UPD_STATE_ID 12
#define SWMGMT_PACKAGE_SETTINGS_ID 13
#define SWMGMT_USER_NAME_ID 14
#define SWMGMT_PASSWORD_ID 15
#define SWMGMT_MAX_ID 16

#define PACKAGE_NAME_LEN CONFIG_LWM2M_SWMGMT_PACKAGE_NAME_LEN
#define PACKAGE_VERSION_LEN CONFIG_LWM2M_SWMGMT_PACKAGE_VERSION_LEN
#define PACKAGE_URI_LEN CONFIG_LWM2M_SWMGMT_PACKAGE_URI_LEN
#define MAX_INSTANCE_COUNT CONFIG_LWM2M_SWMGMT_MAX_INSTANCE_COUNT

/*
 * Calculate resource instances as follows:
 * start with SWMGMT_MAX_ID
 * subtract EXEC resources (4)
 */
#define NR_EXEC_RESOURCES 4
#define RESOURCE_INSTANCE_COUNT (SWMGMT_MAX_ID - NR_EXEC_RESOURCES)

static struct lwm2m_engine_obj swmgmt;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD(SWMGMT_PACKAGE_NAME_ID, R, STRING),
	OBJ_FIELD(SWMGMT_PACKAGE_VERSION_ID, R, STRING),
	OBJ_FIELD(SWMGMT_PACKAGE_ID, W_OPT, OPAQUE),
	OBJ_FIELD(SWMGMT_PACKAGE_URI_ID, W_OPT, STRING),
	OBJ_FIELD_EXECUTE(SWMGMT_INSTALL_ID),
	OBJ_FIELD(SWMGMT_CHECKPOINT_ID, R_OPT, OBJLNK),
	OBJ_FIELD_EXECUTE(SWMGMT_UNINSTALL_ID),
	OBJ_FIELD(SWMGMT_UPDATE_STATE_ID, R, U8),
	OBJ_FIELD(SWMGMT_UPDATE_SUPPORTED_OBJECTS_ID, RW_OPT, BOOL),
	OBJ_FIELD(SWMGMT_UPDATE_RESULT_ID, R, U8),
	OBJ_FIELD_EXECUTE(SWMGMT_ACTIVATE_ID),
	OBJ_FIELD_EXECUTE(SWMGMT_DEACTIVATE_ID),
	OBJ_FIELD(SWMGMT_ACTIVATION_UPD_STATE_ID, R, BOOL),
	OBJ_FIELD(SWMGMT_PACKAGE_SETTINGS_ID, RW_OPT, OBJLNK),
	OBJ_FIELD(SWMGMT_USER_NAME_ID, W_OPT, STRING),
	OBJ_FIELD(SWMGMT_PASSWORD_ID, W_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][SWMGMT_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

#define UPD_STATE_INITIAL 0
#define UPD_STATE_DOWNLOAD_STARTED 1
#define UPD_STATE_DOWNLOADED 2
#define UPD_STATE_DELIVERED 3
#define UPD_STATE_INSTALLED 4

#define EVENT_PKG_URI_WRITE 0
#define EVENT_PKG_WRITTEN 1
#define EVENT_PKG_INTEGRITY_VERIFIED 2
#define EVENT_INSTALL 4
#define EVENT_INSTALL_SUCCESSFUL 5
#define EVENT_INSTALL_FAIL 6
#define EVENT_DELETE_PACKAGE 7
#define EVENT_FOR_UPDATE 8
#define EVENT_DOWNLOAD_FAILED 9
#define EVENT_PKG_INTEGRITY_FAILED 10
#define EVENT_ACTIVATE 11
#define EVENT_DEACTIVATE 12

/*
 * 0: Initial value. Prior to download any new package in the Device, Update
 *    Result MUST be reset to this initial value. One side effect of executing
 *    the Uninstall resource is to reset Update Result to this initial value 0.
 */
#define UPD_RES_INITIAL 0

/* 1: Downloading. The package downloading process is ongoing. */
#define UPD_RES_DOWNLOADING 1

/* 2: Software successfully installed. */
#define UPD_RES_SW_SUCCESSFULLY_INSTALLED 2

/* 3: Successfully Downloaded and package integrity verified */
#define UPD_RES_DOWNLOADED_AND_VERIFIED 3

/* 4-49: reserved for expansion of other scenarios>> */
/* 50: Not enough storage for the new software package. */
#define UPD_RES_NOT_ENOUGH_STORAGE 50

/* 51: Out of memory during downloading process. */
#define UPD_RES_OUT_OF_MEMORY_DURING_DOWNLOAD 51

/* 52: Connection lost during downloading process. */
#define UPD_RES_LOST_CONNECTION_DURING_DOWNLOAD 52

/* 53: Package integrity check failure. */
#define UPD_RES_PACKAGE_INTEGRITY_CHECK_FAILURE 53

/* 54: Unsupported package type. */
#define UPD_RES_UNSUPPORTED_PACKAGE_TYPE 54

/* 55: Undefined */
/* 56: Invalid URI */
#define UPD_RES_INVALID_URI 56

/* 57: Device defined update error */
#define UPD_RES_DEVICE_DEFINED_UPDATE_ERROR 57

/* 58: Software installation failure */
#define UPD_RES_SW_INSTALLATION_FAILURE 58

/* 59: Uninstallation Failure during forUpdate(arg=0) */
#define UPD_RES_UNINSTALLATION_FAILURE_FOR_UPDATE 59

/*
 * 60-200 : reserved for expansion selection to be in blocks depending on new
 *          introduction of features
 */

struct lwm2m_swmgmt_data {
	uint16_t obj_inst_id;

	char package_name[PACKAGE_NAME_LEN];
	char package_version[PACKAGE_VERSION_LEN];

	bool next_package_is_upgrade;

	uint8_t update_state;
	uint8_t update_result;

	bool activation_state;

	lwm2m_engine_get_data_cb_t read_package_cb;
	lwm2m_engine_execute_cb_t install_package_cb;
	lwm2m_engine_user_cb_t upgrade_package_cb;
	lwm2m_engine_execute_cb_t delete_package_cb;
	lwm2m_engine_execute_cb_t activate_cb;
	lwm2m_engine_execute_cb_t deactivate_cb;
	lwm2m_engine_set_data_cb_t write_package_cb;

#ifdef CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
	char package_uri[PACKAGE_URI_LEN];
#endif
};

/* Package pull request should come with a verify_cb which needs to be stored for when package
 * gets downloaded
 */
static int (*verify_package)(void);

static int callback_execute_not_defined(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	LOG_ERR("Callback not defined for inst %u", obj_inst_id);
	return -EINVAL;
}

static int callback_write_not_defined(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				      uint8_t *data, uint16_t data_len, bool last_block,
				      size_t total_size)
{
	LOG_ERR("Callback not defined for inst %u", obj_inst_id);
	return -EINVAL;
}

static void *callback_read_not_defined(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				       size_t *data_len)
{
	LOG_ERR("Callback not defined for inst %u", obj_inst_id);
	return NULL;
}

static struct lwm2m_swmgmt_data swmgmt_data[MAX_INSTANCE_COUNT] = { 0 };

static struct lwm2m_swmgmt_data *find_index(uint16_t obj_inst_id)
{
	int index;

	for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			return &swmgmt_data[index];
		}
	}

	LOG_DBG("No instance found for obj id %u", obj_inst_id);
	return NULL;
}

int lwm2m_swmgmt_set_activate_cb(uint16_t obj_inst_id, lwm2m_engine_execute_cb_t cb)
{
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);
	if (!instance) {
		return -ENOENT;
	}

	if (!cb) {
		cb = callback_execute_not_defined;
	}

	instance->activate_cb = cb;
	return 0;
}

int lwm2m_swmgmt_set_deactivate_cb(uint16_t obj_inst_id, lwm2m_engine_execute_cb_t cb)
{
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);
	if (!instance) {
		return -ENOENT;
	}

	if (!cb) {
		cb = callback_execute_not_defined;
	}

	instance->deactivate_cb = cb;
	return 0;
}

int lwm2m_swmgmt_set_install_package_cb(uint16_t obj_inst_id, lwm2m_engine_execute_cb_t cb)
{
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);
	if (!instance) {
		return -ENOENT;
	}

	if (!cb) {
		cb = callback_execute_not_defined;
	}

	instance->install_package_cb = cb;
	return 0;
}

int lwm2m_swmgmt_set_delete_package_cb(uint16_t obj_inst_id, lwm2m_engine_execute_cb_t cb)
{
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);
	if (!instance) {
		return -ENOENT;
	}

	if (!cb) {
		cb = callback_execute_not_defined;
	}

	instance->delete_package_cb = cb;
	return 0;
}

int lwm2m_swmgmt_set_write_package_cb(uint16_t obj_inst_id, lwm2m_engine_set_data_cb_t cb)
{
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);
	if (!instance) {
		return -ENOENT;
	}

	if (!cb) {
		cb = callback_write_not_defined;
	}

	instance->write_package_cb = cb;
	return 0;
}

int lwm2m_swmgmt_set_read_package_version_cb(uint16_t obj_inst_id, lwm2m_engine_get_data_cb_t cb)
{
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);
	if (!instance) {
		return -ENOENT;
	}

	if (!cb) {
		cb = callback_read_not_defined;
	}

	instance->read_package_cb = cb;
	return 0;
}

void *state_read_pkg_version(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			     size_t *data_len)
{
	struct lwm2m_swmgmt_data *instance = NULL;
	void *result = NULL;

	instance = find_index(obj_inst_id);
	if (!instance) {
		return NULL;
	}

	if (instance->read_package_cb) {
		result = instance->read_package_cb(obj_inst_id, res_id, res_inst_id, data_len);
	}

	return result;
}

static int handle_event(struct lwm2m_swmgmt_data *instance, uint8_t event)
{
	int ret = 0;

	if (!instance) {
		return -EINVAL;
	}

	switch (instance->update_state) {
	case UPD_STATE_INITIAL:
		switch (event) {
		case EVENT_PKG_URI_WRITE:
			instance->update_state = UPD_STATE_DOWNLOAD_STARTED;
			instance->update_result = UPD_RES_DOWNLOADING;
			ret = 0;
			break;
		default:
			ret = -EINVAL;
			break;
		}
		break;
	case UPD_STATE_DOWNLOAD_STARTED:
		switch (event) {
		case EVENT_PKG_WRITTEN:
			instance->update_state = UPD_STATE_DOWNLOADED;
			instance->update_result = UPD_RES_INITIAL;
			break;
		case EVENT_DOWNLOAD_FAILED:
			instance->update_state = UPD_STATE_INITIAL;

			/* Inform the instance of DOWNLOAD_FAILED by calling
			 * write_package_cb with a bunch of NULL parameters
			 */
			instance->write_package_cb(instance->obj_inst_id, 0, 0, NULL, 0, false, 0);
			break;
		default:
			ret = -EINVAL;
			break;
		}
		break;
	case UPD_STATE_DOWNLOADED:
		switch (event) {
		case (EVENT_PKG_INTEGRITY_VERIFIED):
			instance->update_state = UPD_STATE_DELIVERED;
			instance->update_result = UPD_RES_INITIAL;
			break;
		case (EVENT_PKG_INTEGRITY_FAILED):
			instance->update_state = UPD_STATE_INITIAL;
			instance->update_result = UPD_RES_PACKAGE_INTEGRITY_CHECK_FAILURE;
			break;
		default:
			ret = -EINVAL;
			break;
		}
		break;
	case UPD_STATE_DELIVERED:
		switch (event) {
		case EVENT_INSTALL:
			if (instance->next_package_is_upgrade) {
				ret = instance->upgrade_package_cb(instance->obj_inst_id);
			}

			else {
				ret = instance->install_package_cb(instance->obj_inst_id, NULL, 0);
			}

			break;

		case EVENT_INSTALL_SUCCESSFUL:
			instance->update_state = UPD_STATE_INSTALLED;
			instance->update_result = UPD_RES_SW_SUCCESSFULLY_INSTALLED;
			instance->next_package_is_upgrade = false;
			break;

		case EVENT_INSTALL_FAIL:
			instance->update_state = UPD_STATE_DELIVERED;
			instance->update_result = UPD_RES_SW_INSTALLATION_FAILURE;
			break;

		case EVENT_DELETE_PACKAGE:
			ret = instance->delete_package_cb(instance->obj_inst_id, NULL, 0);
			if (ret == 0) {
				instance->update_state = UPD_STATE_INITIAL;
				/* update_result unchanged */
			}
			break;

		default:
			ret = -EINVAL;
			break;
		}
		break;
	case UPD_STATE_INSTALLED:
		switch (event) {
		case EVENT_ACTIVATE:
			ret = instance->activate_cb(instance->obj_inst_id, NULL, 0);
			if (ret == 0) {
				instance->activation_state = true;
			}
			break;
		case EVENT_DEACTIVATE:
			ret = instance->deactivate_cb(instance->obj_inst_id, NULL, 0);
			if (ret == 0) {
				instance->activation_state = false;
			}
			break;
		case EVENT_FOR_UPDATE:
			instance->next_package_is_upgrade = true;
			instance->update_state = UPD_STATE_INITIAL;
			instance->update_result = UPD_RES_INITIAL;
		case EVENT_DELETE_PACKAGE:
			ret = instance->delete_package_cb(instance->obj_inst_id, NULL, 0);
			if (ret == 0) {
				instance->update_state = UPD_STATE_INITIAL;
				instance->update_result = UPD_RES_INITIAL;
			}
			break;
		default:
			ret = -EINVAL;
			break;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int install_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);

	return handle_event(instance, EVENT_INSTALL);
}

int lwm2m_swmgmt_install_completed(uint16_t obj_inst_id, int error_code)
{
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);

	if (error_code) {
		error_code = handle_event(instance, EVENT_INSTALL_FAIL);
	} else {
		error_code = handle_event(instance, EVENT_INSTALL_SUCCESSFUL);
	}

	return error_code;
}

static int uninstall_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);

	return handle_event(instance, EVENT_DELETE_PACKAGE);
}

static int activate_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);

	return handle_event(instance, EVENT_ACTIVATE);
}

static int deactivate_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);

	return handle_event(instance, EVENT_DEACTIVATE);
}

static int package_write_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			    uint8_t *data, uint16_t data_len, bool last_block, size_t total_size)
{
	int ret = -EINVAL;
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);

	ret = handle_event(instance, EVENT_PKG_URI_WRITE);

	if (ret < 0) {
		return ret;
	}

	ret = instance->write_package_cb(obj_inst_id, res_id, res_inst_id, data, data_len,
					 last_block, total_size);

	if (ret < 0) {
		handle_event(instance, EVENT_DOWNLOAD_FAILED);
		switch (ret) {
		case -ENOMEM:
			instance->update_result = UPD_RES_OUT_OF_MEMORY_DURING_DOWNLOAD;
			break;
		case -ENOSPC:
			instance->update_result = UPD_RES_NOT_ENOUGH_STORAGE;
			ret = -EFBIG;
			break;
		case -EFAULT:
			instance->update_result = UPD_RES_PACKAGE_INTEGRITY_CHECK_FAILURE;
			break;
		default:
			instance->update_result = UPD_RES_LOST_CONNECTION_DURING_DOWNLOAD;
			break;
		}

		return ret;
	}

	if (last_block) {
		handle_event(instance, EVENT_PKG_WRITTEN);
	}

	return 0;
}

static void set_update_result(uint16_t obj_inst_id, int error_code)
{
	int ret;
	struct lwm2m_swmgmt_data *instance;

	instance = find_index(obj_inst_id);

	if (error_code == 0) {
		handle_event(instance, EVENT_PKG_WRITTEN);

		/* If the verify function wasn't provided, skip the check. */
		if (verify_package) {
			ret = verify_package();
		} else {
			ret = 0;
		}

		if (ret == 0) {
			handle_event(instance, EVENT_PKG_INTEGRITY_VERIFIED);
		} else {
			handle_event(instance, EVENT_PKG_INTEGRITY_FAILED);
		}

		return;
	}

	handle_event(instance, EVENT_DOWNLOAD_FAILED);
	if (error_code == -ENOMEM) {
		instance->update_result = UPD_RES_OUT_OF_MEMORY_DURING_DOWNLOAD;
	} else if (error_code == -ENOSPC) {
		instance->update_result = UPD_RES_NOT_ENOUGH_STORAGE;
	} else if (error_code == -EFAULT) {
		instance->update_result = UPD_RES_PACKAGE_INTEGRITY_CHECK_FAILURE;
	} else if (error_code == -ENOTSUP) {
		instance->update_result = UPD_RES_INVALID_URI;
	} else {
		instance->update_result = UPD_RES_LOST_CONNECTION_DURING_DOWNLOAD;
	}
}

static int package_uri_write_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				uint8_t *data, uint16_t data_len, bool last_block,
				size_t total_size)
{
#ifdef CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
	int error_code;
	struct lwm2m_swmgmt_data *instance = NULL;

	instance = find_index(obj_inst_id);

	struct requesting_object req = { .obj_inst_id = obj_inst_id,
					 .is_firmware_uri = false,
					 .result_cb = set_update_result,
					 .write_cb = instance->write_package_cb,
					 .verify_cb = NULL };

	verify_package = req.verify_cb;

	error_code = lwm2m_pull_context_start_transfer(instance->package_uri, req, K_NO_WAIT);

	if (error_code) {
		return error_code;
	}

	return handle_event(instance, EVENT_PKG_URI_WRITE);

#else
	return -EINVAL;
#endif
}

static struct lwm2m_engine_obj_inst *swmgmt_create(uint16_t obj_inst_id)
{
	struct lwm2m_swmgmt_data *instance = NULL;
	int index, res_idx = 0, res_inst_idx = 0;

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

	instance = &swmgmt_data[index];

	/* Set default values */
	(void)memset(res[index], 0, sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	(void)memset(instance->package_name, 0, PACKAGE_NAME_LEN);
	(void)memset(instance->package_version, 0, PACKAGE_VERSION_LEN);

	instance->obj_inst_id = obj_inst_id;
	instance->update_state = 0;
	instance->update_result = 0;
	instance->activation_state = false;

	instance->next_package_is_upgrade = false;

	instance->install_package_cb = callback_execute_not_defined;
	instance->delete_package_cb = callback_execute_not_defined;
	instance->activate_cb = callback_execute_not_defined;
	instance->deactivate_cb = callback_execute_not_defined;
	instance->write_package_cb = callback_write_not_defined;

#ifdef CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
	(void)memset(instance->package_uri, 0, PACKAGE_URI_LEN);
#endif

	/* initialize instance resource data */
	INIT_OBJ_RES_DATA_LEN(SWMGMT_PACKAGE_NAME_ID, res[index], res_idx, res_inst[index],
			  res_inst_idx, &instance->package_name, PACKAGE_NAME_LEN, 0);

	INIT_OBJ_RES_LEN(SWMGMT_PACKAGE_VERSION_ID, res[index], res_idx, res_inst[index],
			 res_inst_idx, 1, true, false, &instance->package_version,
			 PACKAGE_VERSION_LEN, 0, state_read_pkg_version, NULL, NULL, NULL, NULL);

	INIT_OBJ_RES_OPT(SWMGMT_PACKAGE_ID, res[index], res_idx, res_inst[index], res_inst_idx, 1,
			 true, false, NULL, NULL, package_write_cb, NULL, NULL);

#ifdef CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
	INIT_OBJ_RES(SWMGMT_PACKAGE_URI_ID, res[index], res_idx, res_inst[index], res_inst_idx, 1,
		     true, true, instance->package_uri, PACKAGE_URI_LEN, NULL, NULL, NULL,
		     package_uri_write_cb, NULL);
#else
	INIT_OBJ_RES_OPT(SWMGMT_PACKAGE_URI_ID, res[index], res_idx, res_inst[index], res_inst_idx,
			 1, true, false, NULL, NULL, package_uri_write_cb, NULL, NULL);
#endif

	INIT_OBJ_RES_EXECUTE(SWMGMT_INSTALL_ID, res[index], res_idx, install_cb);

	INIT_OBJ_RES_OPTDATA(SWMGMT_CHECKPOINT_ID, res[index], res_idx, res_inst[index],
			     res_inst_idx);

	INIT_OBJ_RES_EXECUTE(SWMGMT_UNINSTALL_ID, res[index], res_idx, uninstall_cb);

	INIT_OBJ_RES_DATA(SWMGMT_UPDATE_STATE_ID, res[index], res_idx, res_inst[index],
			  res_inst_idx, &instance->update_state, sizeof(uint8_t));

	INIT_OBJ_RES_OPTDATA(SWMGMT_UPDATE_SUPPORTED_OBJECTS_ID, res[index], res_idx,
			     res_inst[index], res_inst_idx);

	INIT_OBJ_RES_DATA(SWMGMT_UPDATE_RESULT_ID, res[index], res_idx, res_inst[index],
			  res_inst_idx, &instance->update_result, sizeof(uint8_t));

	INIT_OBJ_RES_EXECUTE(SWMGMT_ACTIVATE_ID, res[index], res_idx, activate_cb);
	INIT_OBJ_RES_EXECUTE(SWMGMT_DEACTIVATE_ID, res[index], res_idx, deactivate_cb);

	INIT_OBJ_RES_DATA(SWMGMT_ACTIVATION_UPD_STATE_ID, res[index], res_idx, res_inst[index],
			  res_inst_idx, &instance->activation_state, sizeof(bool));

	INIT_OBJ_RES_OPTDATA(SWMGMT_PACKAGE_SETTINGS_ID, res[index], res_idx, res_inst[index],
			     res_inst_idx);
	INIT_OBJ_RES_OPTDATA(SWMGMT_USER_NAME_ID, res[index], res_idx, res_inst[index],
			     res_inst_idx);
	INIT_OBJ_RES_OPTDATA(SWMGMT_PASSWORD_ID, res[index], res_idx, res_inst[index],
			     res_inst_idx);

	inst[index].resources = res[index];
	inst[index].resource_count = res_idx;
	return &inst[index];
}

static int lwm2m_swmgmt_init(const struct device *dev)
{
	swmgmt.obj_id = LWM2M_OBJECT_SOFTWARE_MANAGEMENT_ID;
	swmgmt.version_major = SWMGMT_VERSION_MAJOR;
	swmgmt.version_minor = SWMGMT_VERSION_MINOR;
	swmgmt.fields = fields;
	swmgmt.field_count = ARRAY_SIZE(fields);
	swmgmt.max_instance_count = MAX_INSTANCE_COUNT;
	swmgmt.create_cb = swmgmt_create;
	lwm2m_register_obj(&swmgmt);

	return 0;
}

SYS_INIT(lwm2m_swmgmt_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
