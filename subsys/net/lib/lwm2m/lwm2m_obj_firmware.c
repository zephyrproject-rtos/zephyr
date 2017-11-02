/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "lwm2m_obj_firmware"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>
#include <net/coap.h>
#include <string.h>
#include <init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

/* Firmware resource IDs */
#define FIRMWARE_PACKAGE_ID			0
#define FIRMWARE_PACKAGE_URI_ID			1
#define FIRMWARE_UPDATE_ID			2
#define FIRMWARE_STATE_ID			3
#define FIRMWARE_UPDATE_RESULT_ID		5
#define FIRMWARE_PACKAGE_NAME_ID		6 /* TODO */
#define FIRMWARE_PACKAGE_VERSION_ID		7 /* TODO */
#define FIRMWARE_UPDATE_PROTO_SUPPORT_ID	8 /* TODO */
#define FIRMWARE_UPDATE_DELIV_METHOD_ID		9

#define FIRMWARE_MAX_ID				10

#define DELIVERY_METHOD_PULL_ONLY		0
#define DELIVERY_METHOD_PUSH_ONLY		1
#define DELIVERY_METHOD_BOTH			2

#define PACKAGE_URI_LEN				255

/* resource state variables */
static u8_t update_state;
static u8_t update_result;
static u8_t delivery_method;
static char package_uri[PACKAGE_URI_LEN];

/* only 1 instance of firmware object exists */
static struct lwm2m_engine_obj firmware;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD(FIRMWARE_PACKAGE_ID, W, OPAQUE, 0),
	OBJ_FIELD(FIRMWARE_PACKAGE_URI_ID, RW, STRING, 0),
	OBJ_FIELD_EXECUTE(FIRMWARE_UPDATE_ID),
	OBJ_FIELD_DATA(FIRMWARE_STATE_ID, R, U8),
	OBJ_FIELD_DATA(FIRMWARE_UPDATE_RESULT_ID, R, U8),
	OBJ_FIELD_DATA(FIRMWARE_UPDATE_DELIV_METHOD_ID, R, U8)
};

static struct lwm2m_engine_obj_inst inst;
static struct lwm2m_engine_res_inst res[FIRMWARE_MAX_ID];

static lwm2m_engine_set_data_cb_t write_cb;
static lwm2m_engine_exec_cb_t update_cb;

#ifdef CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
extern int lwm2m_firmware_start_transfer(char *package_uri);
#endif

u8_t lwm2m_firmware_get_update_state(void)
{
	return update_state;
}

void lwm2m_firmware_set_update_state(u8_t state)
{
	bool error = false;

	/* Check LWM2M SPEC appendix E.6.1 */
	switch (state) {
	case STATE_DOWNLOADING:
		if (update_state != STATE_IDLE) {
			error = true;
		}
		break;
	case STATE_DOWNLOADED:
		if (update_state != STATE_DOWNLOADING &&
		    update_state != STATE_UPDATING) {
			error = true;
		}
		break;
	case STATE_UPDATING:
		if (update_state != STATE_DOWNLOADED) {
			error = true;
		}
		break;
	case STATE_IDLE:
		break;
	default:
		SYS_LOG_ERR("Unhandled state: %u", state);
		return;
	}

	if (error) {
		SYS_LOG_ERR("Invalid state transition: %u -> %u",
			    update_state, state);
	}

	update_state = state;
	NOTIFY_OBSERVER(LWM2M_OBJECT_FIRMWARE_ID, 0, FIRMWARE_STATE_ID);
	SYS_LOG_DBG("Update state = %d", update_state);
}

u8_t lwm2m_firmware_get_update_result(void)
{
	return update_result;
}

void lwm2m_firmware_set_update_result(u8_t result)
{
	u8_t state;
	bool error = false;

	/* Check LWM2M SPEC appendix E.6.1 */
	switch (result) {
	case RESULT_DEFAULT:
		lwm2m_firmware_set_update_state(STATE_IDLE);
		break;
	case RESULT_SUCCESS:
		if (update_state != STATE_UPDATING) {
			error = true;
			state = update_state;
		}

		lwm2m_firmware_set_update_state(STATE_IDLE);
		break;
	case RESULT_NO_STORAGE:
	case RESULT_OUT_OF_MEM:
	case RESULT_CONNECTION_LOST:
	case RESULT_UNSUP_FW:
	case RESULT_INVALID_URI:
	case RESULT_UNSUP_PROTO:
		if (update_state != STATE_DOWNLOADING) {
			error = true;
			state = update_state;
		}

		lwm2m_firmware_set_update_state(STATE_IDLE);
		break;
	case RESULT_INTEGRITY_FAILED:
		if (update_state != STATE_DOWNLOADING &&
		    update_state != STATE_UPDATING) {
			error = true;
			state = update_state;
		}

		lwm2m_firmware_set_update_state(STATE_IDLE);
		break;
	case RESULT_UPDATE_FAILED:
		if (update_state != STATE_UPDATING) {
			error = true;
			state = update_state;
		}

		/* Next state could be idle or downloaded */
		break;
	default:
		SYS_LOG_ERR("Unhandled result: %u", result);
		return;
	}

	if (error) {
		SYS_LOG_ERR("Unexpected result(%u) set while state is %u",
			    result, state);
	}

	update_result = result;
	NOTIFY_OBSERVER(LWM2M_OBJECT_FIRMWARE_ID, 0, FIRMWARE_UPDATE_RESULT_ID);
	SYS_LOG_DBG("Update result = %d", update_result);
}

static int package_write_cb(u16_t obj_inst_id,
			    u8_t *data, u16_t data_len,
			    bool last_block, size_t total_size)
{
	u8_t state;
	int ret;

	state = lwm2m_firmware_get_update_state();
	if (state == STATE_IDLE) {
		/* TODO: setup timer to check download status,
		 * make sure it fail after timeout
		 */
		lwm2m_firmware_set_update_state(STATE_DOWNLOADING);
	} else if (state != STATE_DOWNLOADING) {
		if (data_len == 0 && state == STATE_DOWNLOADED) {
			/* reset to state idle and result default */
			lwm2m_firmware_set_update_result(RESULT_DEFAULT);
			return 0;
		}

		SYS_LOG_DBG("Cannot download: state = %d", state);
		return -EPERM;
	}

	ret = write_cb ? write_cb(obj_inst_id, data, data_len,
				  last_block, total_size) : 0;
	if (ret >= 0) {
		if (last_block) {
			lwm2m_firmware_set_update_state(STATE_DOWNLOADED);
		}

		return 0;
	} else if (ret == -ENOMEM) {
		lwm2m_firmware_set_update_result(RESULT_OUT_OF_MEM);
	} else if (ret == -ENOSPC) {
		lwm2m_firmware_set_update_result(RESULT_NO_STORAGE);
		/* Response 4.13 (RFC7959, section 2.9.3) */
		/* TODO: should include size1 option to indicate max size */
		ret = -EFBIG;
	} else if (ret == -EFAULT) {
		lwm2m_firmware_set_update_result(RESULT_INTEGRITY_FAILED);
	} else {
		lwm2m_firmware_set_update_result(RESULT_UPDATE_FAILED);
	}

	return ret;
}

static int package_uri_write_cb(u16_t obj_inst_id,
				u8_t *data, u16_t data_len,
				bool last_block, size_t total_size)
{
	SYS_LOG_DBG("PACKAGE_URI WRITE: %s", package_uri);

#ifdef CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
	u8_t state = lwm2m_firmware_get_update_state();

	if (state == STATE_IDLE) {
		lwm2m_firmware_set_update_result(RESULT_DEFAULT);
		lwm2m_firmware_start_transfer(package_uri);
	} else if (state == STATE_DOWNLOADED && data_len == 0) {
		/* reset to state idle and result default */
		lwm2m_firmware_set_update_result(RESULT_DEFAULT);
	}

	return 0;
#else
	return -EINVAL;
#endif
}

void lwm2m_firmware_set_write_cb(lwm2m_engine_set_data_cb_t cb)
{
	write_cb = cb;
}

lwm2m_engine_set_data_cb_t lwm2m_firmware_get_write_cb(void)
{
	return write_cb;
}

void lwm2m_firmware_set_update_cb(lwm2m_engine_exec_cb_t cb)
{
	update_cb = cb;
}

lwm2m_engine_exec_cb_t lwm2m_firmware_get_update_cb(void)
{
	return update_cb;
}

static int firmware_update_cb(u16_t obj_inst_id)
{
	lwm2m_engine_exec_cb_t callback;
	u8_t state;
	int ret;

	state = lwm2m_firmware_get_update_state();
	if (state != STATE_DOWNLOADED) {
		SYS_LOG_ERR("State other than downloaded: %d", state);
		return -EPERM;
	}

	lwm2m_firmware_set_update_state(STATE_UPDATING);

	callback = lwm2m_firmware_get_update_cb();
	if (callback) {
		ret = callback(obj_inst_id);
		if (ret < 0) {
			SYS_LOG_ERR("Failed to update firmware: %d", ret);
			lwm2m_firmware_set_update_result(
				ret == -EINVAL ? RESULT_INTEGRITY_FAILED :
						 RESULT_UPDATE_FAILED);
			return 0;
		}
	}

	return 0;
}

static struct lwm2m_engine_obj_inst *firmware_create(u16_t obj_inst_id)
{
	int i = 0;

	/* initialize instance resource data */
	INIT_OBJ_RES(res, i, FIRMWARE_PACKAGE_ID, 0, NULL, 0,
		     NULL, NULL, package_write_cb, NULL);
	INIT_OBJ_RES(res, i, FIRMWARE_PACKAGE_URI_ID, 0,
		     package_uri, PACKAGE_URI_LEN,
		     NULL, NULL, package_uri_write_cb, NULL);
	INIT_OBJ_RES_EXECUTE(res, i, FIRMWARE_UPDATE_ID,
			     firmware_update_cb);
	INIT_OBJ_RES_DATA(res, i, FIRMWARE_STATE_ID,
			  &update_state, sizeof(update_state));
	INIT_OBJ_RES_DATA(res, i, FIRMWARE_UPDATE_RESULT_ID,
			  &update_result, sizeof(update_result));
	INIT_OBJ_RES_DATA(res, i, FIRMWARE_UPDATE_DELIV_METHOD_ID,
			  &delivery_method, sizeof(delivery_method));

	inst.resources = res;
	inst.resource_count = i;
	SYS_LOG_DBG("Create LWM2M firmware instance: %d", obj_inst_id);
	return &inst;
}

static int lwm2m_firmware_init(struct device *dev)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	/* Set default values */
	package_uri[0] = '\0';
	/* Initialize state machine */
	/* TODO: should be restored from the permanent storage */
	update_state = STATE_IDLE;
	update_result = RESULT_DEFAULT;
#ifdef CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
	delivery_method = DELIVERY_METHOD_BOTH;
#else
	delivery_method = DELIVERY_METHOD_PUSH_ONLY;
#endif

	firmware.obj_id = LWM2M_OBJECT_FIRMWARE_ID;
	firmware.fields = fields;
	firmware.field_count = sizeof(fields) / sizeof(*fields);
	firmware.max_instance_count = 1;
	firmware.create_cb = firmware_create;
	lwm2m_register_obj(&firmware);

	/* auto create the only instance */
	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_FIRMWARE_ID, 0, &obj_inst);
	if (ret < 0) {
		SYS_LOG_DBG("Create LWM2M instance 0 error: %d", ret);
	}

	return ret;
}

SYS_INIT(lwm2m_firmware_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
