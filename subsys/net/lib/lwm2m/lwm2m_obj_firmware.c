/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_obj_firmware
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <stdio.h>
#include <zephyr/init.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_pull_context.h"

#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT_MULTIPLE)
#define MAX_INSTANCE_COUNT CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_INSTANCE_COUNT
#else
#define MAX_INSTANCE_COUNT 1
#endif

/* Firmware resource IDs */
#define FIRMWARE_PACKAGE_ID			0
#define FIRMWARE_PACKAGE_URI_ID			1
#define FIRMWARE_UPDATE_ID			2
#define FIRMWARE_STATE_ID			3
#define FIRMWARE_UPDATE_RESULT_ID		5
#define FIRMWARE_PACKAGE_NAME_ID		6
#define FIRMWARE_PACKAGE_VERSION_ID		7
#define FIRMWARE_UPDATE_PROTO_SUPPORT_ID	8
#define FIRMWARE_UPDATE_DELIV_METHOD_ID		9

#define FIRMWARE_MAX_ID				10

#define DELIVERY_METHOD_PULL_ONLY		0
#define DELIVERY_METHOD_PUSH_ONLY		1
#define DELIVERY_METHOD_BOTH			2

/*
 * Calculate resource instances as follows:
 * start with FIRMWARE_MAX_ID
 * subtract EXEC resources (1)
 */
#define RESOURCE_INSTANCE_COUNT	(FIRMWARE_MAX_ID - 1)

/* resource state variables */
static uint8_t update_state[MAX_INSTANCE_COUNT];
static uint8_t update_result[MAX_INSTANCE_COUNT];
static uint8_t delivery_method[MAX_INSTANCE_COUNT];
static char package_uri[MAX_INSTANCE_COUNT][LWM2M_PACKAGE_URI_LEN];

/* A varying number of firmware object exists */
static struct lwm2m_engine_obj firmware;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(FIRMWARE_PACKAGE_ID, W, OPAQUE),
	OBJ_FIELD_DATA(FIRMWARE_PACKAGE_URI_ID, RW, STRING),
	OBJ_FIELD_EXECUTE(FIRMWARE_UPDATE_ID),
	OBJ_FIELD_DATA(FIRMWARE_STATE_ID, R, U8),
	OBJ_FIELD_DATA(FIRMWARE_UPDATE_RESULT_ID, R, U8),
	OBJ_FIELD_DATA(FIRMWARE_PACKAGE_NAME_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(FIRMWARE_PACKAGE_VERSION_ID, R_OPT, STRING),
	OBJ_FIELD_DATA(FIRMWARE_UPDATE_PROTO_SUPPORT_ID, R_OPT, U8),
	OBJ_FIELD_DATA(FIRMWARE_UPDATE_DELIV_METHOD_ID, R, U8)
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][FIRMWARE_MAX_ID];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static lwm2m_engine_set_data_cb_t write_cb[MAX_INSTANCE_COUNT];
static lwm2m_engine_execute_cb_t update_cb[MAX_INSTANCE_COUNT];
static lwm2m_engine_user_cb_t cancel_cb[MAX_INSTANCE_COUNT];

/* Set while a push (write to Package) is downloading; a pull leaves it clear */
static bool push_active[MAX_INSTANCE_COUNT];
/* Uptime at which an idle push is given up, 0 when not armed */
static int64_t push_deadline[MAX_INSTANCE_COUNT];

#define PUSH_TIMEOUT_MS ((int64_t)CONFIG_LWM2M_FIRMWARE_UPDATE_PUSH_TIMEOUT * MSEC_PER_SEC)

#ifdef CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
extern int lwm2m_firmware_start_transfer(uint16_t obj_inst_id, char *package_uri);
#endif

static void push_timeout_service(struct k_work *work);

/* Run the service at the earliest armed deadline, or drop it when none is left.
 * Called with the registry lock held.
 */
static void push_timeout_reschedule(void)
{
	int64_t next = 0;

	for (int i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (push_deadline[i] != 0 && (next == 0 || push_deadline[i] < next)) {
			next = push_deadline[i];
		}
	}

	if (next == 0) {
		(void)lwm2m_engine_update_service_period(push_timeout_service, 0);
	} else {
		(void)lwm2m_engine_call_at(push_timeout_service, next);
	}
}

static void push_timeout_arm(uint16_t obj_inst_id)
{
	if (PUSH_TIMEOUT_MS == 0) {
		return;
	}

	lwm2m_registry_lock();
	push_deadline[obj_inst_id] = k_uptime_get() + PUSH_TIMEOUT_MS;
	push_timeout_reschedule();
	lwm2m_registry_unlock();
}

static void push_end(uint16_t obj_inst_id)
{
	lwm2m_registry_lock();
	push_active[obj_inst_id] = false;
	if (push_deadline[obj_inst_id] != 0) {
		push_deadline[obj_inst_id] = 0;
		push_timeout_reschedule();
	}
	lwm2m_registry_unlock();
}

/* Let the application drop the image. It may erase flash or take its own
 * locks, so the timeout service calls this without holding the registry lock.
 */
static void cancel_notify(uint16_t obj_inst_id)
{
	lwm2m_engine_user_cb_t cancel_callback;

	cancel_callback = lwm2m_firmware_get_cancel_cb_inst(obj_inst_id);
	if (cancel_callback) {
		(void)cancel_callback(obj_inst_id);
	}
}

/* Return to Idle with the given result and let the application drop the image */
static void download_cancel(uint16_t obj_inst_id, uint8_t result)
{
	lwm2m_firmware_set_update_result_inst(obj_inst_id, result);
	cancel_notify(obj_inst_id);
}

static void push_timeout_service(struct k_work *work)
{
	int64_t now = k_uptime_get();
	bool expired[MAX_INSTANCE_COUNT] = { false };

	lwm2m_registry_lock();

	for (uint16_t i = 0; i < MAX_INSTANCE_COUNT; i++) {
		/* Decided under the lock: a push that just ended keeps its image */
		if (push_deadline[i] == 0 || now < push_deadline[i]) {
			continue;
		}

		LOG_WRN("Firmware push to instance %u timed out", i);
		lwm2m_firmware_set_update_result_inst(i, RESULT_CONNECTION_LOST);
		expired[i] = true;
	}

	/* Deadlines that moved on, or belong to other instances, need a new wake-up */
	push_timeout_reschedule();

	lwm2m_registry_unlock();

	/* Server writes run on this thread, so only an application writing Package
	 * from its own thread could start a new push before the old image is dropped.
	 */
	for (uint16_t i = 0; i < MAX_INSTANCE_COUNT; i++) {
		if (expired[i]) {
			cancel_notify(i);
		}
	}
}

uint8_t lwm2m_firmware_get_update_state_inst(uint16_t obj_inst_id)
{
	return update_state[obj_inst_id];
}

uint8_t lwm2m_firmware_get_update_state(void)
{
	return lwm2m_firmware_get_update_state_inst(0);
}

void lwm2m_firmware_set_update_state_inst(uint16_t obj_inst_id, uint8_t state)
{
	bool error = false;
	struct lwm2m_obj_path path = LWM2M_OBJ(LWM2M_OBJECT_FIRMWARE_ID, obj_inst_id,
					       FIRMWARE_UPDATE_RESULT_ID);

	lwm2m_registry_lock();
	/* Check LwM2M SPEC appendix E.6.1 */
	switch (state) {
	case STATE_DOWNLOADING:
		if (update_state[obj_inst_id] == STATE_IDLE) {
			lwm2m_set_u8(&path, RESULT_DEFAULT);
		} else {
			error = true;
		}
		break;
	case STATE_DOWNLOADED:
		if (update_state[obj_inst_id] == STATE_DOWNLOADING) {
			lwm2m_set_u8(&path, RESULT_DEFAULT);
		} else if (update_state[obj_inst_id] == STATE_UPDATING) {
			lwm2m_set_u8(&path, RESULT_UPDATE_FAILED);
		} else {
			error = true;
		}
		break;
	case STATE_UPDATING:
		if (update_state[obj_inst_id] != STATE_DOWNLOADED) {
			error = true;
		}
		break;
	case STATE_IDLE:
		break;
	default:
		LOG_ERR("Unhandled state: %u", state);
		lwm2m_registry_unlock();
		return;
	}

	if (error) {
		LOG_ERR("Invalid state transition: %u -> %u",
			update_state[obj_inst_id], state);
	}

	path.res_id = FIRMWARE_STATE_ID;

	lwm2m_set_u8(&path, state);
	if (state != STATE_DOWNLOADING) {
		push_end(obj_inst_id);
	}
	lwm2m_registry_unlock();

	LOG_DBG("Update state = %d", state);
}

void lwm2m_firmware_set_update_state(uint8_t state)
{
	lwm2m_firmware_set_update_state_inst(0, state);
}

uint8_t lwm2m_firmware_get_update_result_inst(uint16_t obj_inst_id)
{
	return update_result[obj_inst_id];
}

uint8_t lwm2m_firmware_get_update_result(void)
{
	return lwm2m_firmware_get_update_result_inst(0);
}

void lwm2m_firmware_set_update_result_inst(uint16_t obj_inst_id, uint8_t result)
{
	uint8_t state;
	bool error = false;
	struct lwm2m_obj_path path = LWM2M_OBJ(LWM2M_OBJECT_FIRMWARE_ID, obj_inst_id,
					       FIRMWARE_UPDATE_RESULT_ID);

	lwm2m_registry_lock();
	/* Check LwM2M SPEC appendix E.6.1 */
	switch (result) {
	case RESULT_DEFAULT:
		lwm2m_firmware_set_update_state_inst(obj_inst_id, STATE_IDLE);
		break;
	case RESULT_SUCCESS:
		if (update_state[obj_inst_id] != STATE_UPDATING) {
			error = true;
			state = update_state[obj_inst_id];
		}

		lwm2m_firmware_set_update_state_inst(obj_inst_id, STATE_IDLE);
		break;
	case RESULT_NO_STORAGE:
	case RESULT_OUT_OF_MEM:
	case RESULT_CONNECTION_LOST:
	case RESULT_UNSUP_FW:
	case RESULT_INVALID_URI:
	case RESULT_UNSUP_PROTO:
		if (update_state[obj_inst_id] != STATE_DOWNLOADING) {
			error = true;
			state = update_state[obj_inst_id];
		}

		lwm2m_firmware_set_update_state_inst(obj_inst_id, STATE_IDLE);
		break;
	case RESULT_INTEGRITY_FAILED:
		if (update_state[obj_inst_id] != STATE_DOWNLOADING &&
		    update_state[obj_inst_id] != STATE_UPDATING) {
			error = true;
			state = update_state[obj_inst_id];
		}

		lwm2m_firmware_set_update_state_inst(obj_inst_id, STATE_IDLE);
		break;
	case RESULT_UPDATE_FAILED:
		if (update_state[obj_inst_id] != STATE_DOWNLOADING &&
		    update_state[obj_inst_id] != STATE_UPDATING) {
			error = true;
			state = update_state[obj_inst_id];
		}

		lwm2m_firmware_set_update_state_inst(obj_inst_id, STATE_IDLE);
		break;
	default:
		LOG_ERR("Unhandled result: %u", result);
		lwm2m_registry_unlock();
		return;
	}

	if (error) {
		LOG_ERR("Unexpected result(%u) set while state is %u",
			result, state);
	}

	lwm2m_set_u8(&path, result);
	lwm2m_registry_unlock();
	LOG_DBG("Update result = %d", result);
}

void lwm2m_firmware_set_update_result(uint8_t result)
{
	lwm2m_firmware_set_update_result_inst(0, result);
}

/* An empty or NULL ('\0') Package write resets the state machine; a block-wise
 * transfer can only produce such a payload past offset zero.
 */
static bool is_reset_write(const uint8_t *data, uint16_t data_len, size_t offset)
{
	return offset == 0 && (data_len == 0U || (data_len == 1U && data[0] == '\0'));
}

static int package_write_cb(uint16_t obj_inst_id, uint16_t res_id,
			    uint16_t res_inst_id, uint8_t *data,
			    uint16_t data_len, bool last_block,
			    size_t total_size, size_t offset)
{
	uint8_t state;
	bool pull;
	int ret = 0;
	lwm2m_engine_set_data_cb_t write_callback;

	lwm2m_registry_lock();
	state = lwm2m_firmware_get_update_state_inst(obj_inst_id);
	pull = state == STATE_DOWNLOADING && !push_active[obj_inst_id];
	lwm2m_registry_unlock();

	if (pull) {
		LOG_DBG("Package is not writable while a pull is in progress");
		return -EPERM;
	}

	if (state != STATE_UPDATING && is_reset_write(data, data_len, offset)) {
		if (state == STATE_IDLE) {
			lwm2m_firmware_set_update_result_inst(obj_inst_id, RESULT_DEFAULT);
		} else {
			download_cancel(obj_inst_id, RESULT_DEFAULT);
		}
		LOG_DBG("Update canceled by writing %d bytes", data_len);
		return 0;
	}

	if (state == STATE_IDLE) {
		if (offset != 0) {
			/* A late block of a download that was already given up */
			LOG_DBG("Push block at offset %zu without a start", offset);
			return -EFAULT;
		}

		lwm2m_registry_lock();
		lwm2m_firmware_set_update_state_inst(obj_inst_id, STATE_DOWNLOADING);
		push_active[obj_inst_id] = true;
		lwm2m_registry_unlock();
	} else if (state == STATE_DOWNLOADED) {
		LOG_WRN("Download has already completed");
		return -EPERM;
	} else if (state != STATE_DOWNLOADING) {
		LOG_WRN("Cannot download: state = %d", state);
		return -EPERM;
	}

	push_timeout_arm(obj_inst_id);

	write_callback = lwm2m_firmware_get_write_cb_inst(obj_inst_id);
	if (write_callback) {
		ret = write_callback(obj_inst_id, res_id, res_inst_id, data, data_len, last_block,
				     total_size, offset);
	}

	if (ret >= 0) {
		if (last_block) {
			lwm2m_firmware_set_update_state_inst(obj_inst_id, STATE_DOWNLOADED);
		}

		return 0;
	} else if (ret == -ENOMEM) {
		lwm2m_firmware_set_update_result_inst(obj_inst_id, RESULT_OUT_OF_MEM);
	} else if (ret == -ENOSPC) {
		lwm2m_firmware_set_update_result_inst(obj_inst_id, RESULT_NO_STORAGE);
		/* Response 4.13 (RFC7959, section 2.9.3) */
		/* TODO: should include size1 option to indicate max size */
		ret = -EFBIG;
	} else if (ret == -EFAULT) {
		lwm2m_firmware_set_update_result_inst(obj_inst_id, RESULT_INTEGRITY_FAILED);
	} else if (ret == -ENOMSG) {
		lwm2m_firmware_set_update_result_inst(obj_inst_id, RESULT_UNSUP_FW);
	} else {
		lwm2m_firmware_set_update_result_inst(obj_inst_id, RESULT_UPDATE_FAILED);
	}

	return ret;
}

/* The engine records the length of whatever it last wrote, including a write
 * that was diverted below; report the stored URI as it is instead.
 */
static void *package_uri_read_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				 size_t *data_len)
{
	*data_len = strlen(package_uri[obj_inst_id]) + 1;
	return package_uri[obj_inst_id];
}

/* Outside Idle a Package URI write is refused or only resets the state machine,
 * so it must not replace the stored URI: divert it to a scratch buffer.
 */
static void *package_uri_pre_write_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				      size_t *data_len)
{
	static char scratch[LWM2M_PACKAGE_URI_LEN];

	if (lwm2m_firmware_get_update_state_inst(obj_inst_id) == STATE_IDLE) {
		*data_len = sizeof(package_uri[obj_inst_id]);
		return package_uri[obj_inst_id];
	}

	*data_len = sizeof(scratch);
	return scratch;
}

static int package_uri_write_cb(uint16_t obj_inst_id, uint16_t res_id,
				uint16_t res_inst_id, uint8_t *data,
				uint16_t data_len, bool last_block,
				size_t total_size, size_t offset)
{
	LOG_DBG("PACKAGE_URI WRITE: %s", (char *)data);

#ifdef CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
	uint8_t state;
	bool pull;
	bool empty_uri;

	/* writes every block of a block-wise transfer to the start of
	 * the buffer, so it never holds the assembled URI. Reject the
	 * write rather than act on whichever fragment happens to be present.
	 */
	if (!last_block) {
		LOG_ERR("PACKAGE_URI: block-wise write is not supported");
		return -EFBIG;
	}

	lwm2m_registry_lock();
	state = lwm2m_firmware_get_update_state_inst(obj_inst_id);
	pull = state == STATE_DOWNLOADING && !push_active[obj_inst_id];
	lwm2m_registry_unlock();
	empty_uri = data_len == 0 || strnlen(data, data_len) == 0;

	if (empty_uri) {
		/* Nothing can stop a pull or an update yet; tell the server rather than pretend */
		if (pull || state == STATE_UPDATING) {
			LOG_DBG("Package URI cannot be reset: state = %d", state);
			return -EPERM;
		}

		if (state == STATE_IDLE) {
			lwm2m_firmware_set_update_result_inst(obj_inst_id, RESULT_DEFAULT);
		} else {
			download_cancel(obj_inst_id, RESULT_DEFAULT);
		}

		/* The write may have been diverted; the resource reads as empty either way */
		package_uri[obj_inst_id][0] = '\0';

		return 0;
	}

	if (state != STATE_IDLE) {
		LOG_DBG("Cannot start download: state = %d", state);
		return -EPERM;
	}

	lwm2m_firmware_set_update_state_inst(obj_inst_id, STATE_DOWNLOADING);
	lwm2m_firmware_start_transfer(obj_inst_id, package_uri[obj_inst_id]);

	return 0;
#else
	return -EINVAL;
#endif
}

void lwm2m_firmware_set_write_cb(lwm2m_engine_set_data_cb_t cb)
{
	lwm2m_firmware_set_write_cb_inst(0, cb);
}

lwm2m_engine_set_data_cb_t lwm2m_firmware_get_write_cb(void)
{
	return lwm2m_firmware_get_write_cb_inst(0);
}

void lwm2m_firmware_set_update_cb(lwm2m_engine_execute_cb_t cb)
{
	lwm2m_firmware_set_update_cb_inst(0, cb);
}

lwm2m_engine_execute_cb_t lwm2m_firmware_get_update_cb(void)
{
	return lwm2m_firmware_get_update_cb_inst(0);
}

void lwm2m_firmware_set_cancel_cb(lwm2m_engine_user_cb_t cb)
{
	lwm2m_firmware_set_cancel_cb_inst(0, cb);
}

lwm2m_engine_user_cb_t lwm2m_firmware_get_cancel_cb(void)
{
	return lwm2m_firmware_get_cancel_cb_inst(0);
}

void lwm2m_firmware_set_write_cb_inst(uint16_t obj_inst_id, lwm2m_engine_set_data_cb_t cb)
{
	write_cb[obj_inst_id] = cb;
}

lwm2m_engine_set_data_cb_t lwm2m_firmware_get_write_cb_inst(uint16_t obj_inst_id)
{
	return write_cb[obj_inst_id];
}

void lwm2m_firmware_set_update_cb_inst(uint16_t obj_inst_id, lwm2m_engine_execute_cb_t cb)
{
	update_cb[obj_inst_id] = cb;
}

lwm2m_engine_execute_cb_t lwm2m_firmware_get_update_cb_inst(uint16_t obj_inst_id)
{
	return update_cb[obj_inst_id];
}

void lwm2m_firmware_set_cancel_cb_inst(uint16_t obj_inst_id, lwm2m_engine_user_cb_t cb)
{
	cancel_cb[obj_inst_id] = cb;
}

lwm2m_engine_user_cb_t lwm2m_firmware_get_cancel_cb_inst(uint16_t obj_inst_id)
{
	return cancel_cb[obj_inst_id];
}

static int firmware_update_cb(uint16_t obj_inst_id,
			      uint8_t *args, uint16_t args_len)
{
	lwm2m_engine_execute_cb_t callback;
	uint8_t state;
	int ret;

	state = lwm2m_firmware_get_update_state_inst(obj_inst_id);
	if (state != STATE_DOWNLOADED) {
		LOG_ERR("State other than downloaded: %d", state);
		return -EPERM;
	}

	lwm2m_firmware_set_update_state_inst(obj_inst_id, STATE_UPDATING);

	callback = lwm2m_firmware_get_update_cb_inst(obj_inst_id);
	if (callback) {
		ret = callback(obj_inst_id, args, args_len);
		if (ret < 0) {
			LOG_ERR("Failed to update firmware: %d", ret);
			lwm2m_firmware_set_update_result_inst(obj_inst_id,
				ret == -EINVAL ? RESULT_INTEGRITY_FAILED :
						 RESULT_UPDATE_FAILED);
			return 0;
		}
	}

	return 0;
}

static struct lwm2m_engine_obj_inst *firmware_create(uint16_t obj_inst_id)
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

	init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

	/* initialize instance resource data */
	INIT_OBJ_RES_OPT(FIRMWARE_PACKAGE_ID, res[index], i, res_inst[index], j, 1,
			 false, true, NULL, NULL, NULL, package_write_cb, NULL);
	INIT_OBJ_RES_LEN(FIRMWARE_PACKAGE_URI_ID, res[index], i, res_inst[index], j, 1,
		     false, true, package_uri[index], LWM2M_PACKAGE_URI_LEN, 0,
		     package_uri_read_cb, package_uri_pre_write_cb, NULL, package_uri_write_cb,
		     NULL);
	INIT_OBJ_RES_EXECUTE(FIRMWARE_UPDATE_ID, res[index], i, firmware_update_cb);
	INIT_OBJ_RES_DATA(FIRMWARE_STATE_ID, res[index], i, res_inst[index], j,
			  &(update_state[index]), sizeof(update_state[index]));
	INIT_OBJ_RES_DATA(FIRMWARE_UPDATE_RESULT_ID, res[index], i, res_inst[index], j,
			  &(update_result[index]), sizeof(update_result[index]));
	INIT_OBJ_RES_OPTDATA(FIRMWARE_PACKAGE_NAME_ID, res[index], i,
			     res_inst[index], j);
	INIT_OBJ_RES_OPTDATA(FIRMWARE_PACKAGE_VERSION_ID, res[index], i,
			     res_inst[index], j);
	INIT_OBJ_RES_MULTI_OPTDATA(FIRMWARE_UPDATE_PROTO_SUPPORT_ID, res[index], i,
				 res_inst[index], j, 1, false);
	INIT_OBJ_RES_DATA(FIRMWARE_UPDATE_DELIV_METHOD_ID, res[index], i,
			  res_inst[index], j, &(delivery_method[index]),
			  sizeof(delivery_method[index]));

	inst[index].resources = res[index];
	inst[index].resource_count = i;

	LOG_DBG("Create LwM2M firmware instance: %d", obj_inst_id);
	return &inst[index];
}

static int lwm2m_firmware_init(void)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int ret = 0;

	/* Set default values */
	firmware.obj_id = LWM2M_OBJECT_FIRMWARE_ID;
	firmware.version_major = FIRMWARE_VERSION_MAJOR;
	firmware.version_minor = FIRMWARE_VERSION_MINOR;
	firmware.is_core = true;
	firmware.fields = fields;
	firmware.field_count = ARRAY_SIZE(fields);
	firmware.max_instance_count = MAX_INSTANCE_COUNT;
	firmware.create_cb = firmware_create;
	lwm2m_register_obj(&firmware);


	for (int idx = 0; idx < MAX_INSTANCE_COUNT; idx++) {
		package_uri[idx][0] = '\0';

		/* Initialize state machine */
		/* TODO: should be restored from the permanent storage */
		update_state[idx] = STATE_IDLE;
		update_result[idx] = RESULT_DEFAULT;
#ifdef CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
		delivery_method[idx] = DELIVERY_METHOD_BOTH;
#else
		delivery_method[idx] = DELIVERY_METHOD_PUSH_ONLY;
#endif
		ret = lwm2m_create_obj_inst(LWM2M_OBJECT_FIRMWARE_ID, idx, &obj_inst);
		if (ret < 0) {
			LOG_DBG("Create LwM2M instance %d error: %d", idx, ret);
			break;
		}
	}

	return ret;
}

LWM2M_CORE_INIT(lwm2m_firmware_init);
