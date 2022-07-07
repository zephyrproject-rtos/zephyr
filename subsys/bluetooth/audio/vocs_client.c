/*  Bluetooth VOCS - Volume Offset Control Service - Client */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/types.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/vocs.h>

#include "vocs_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_VOCS_CLIENT)
#define LOG_MODULE_NAME bt_vocs_client
#include "common/log.h"

static struct bt_vocs vocs_insts[CONFIG_BT_MAX_CONN * CONFIG_BT_VOCS_CLIENT_MAX_INSTANCE_COUNT];

static struct bt_vocs *lookup_vocs_by_handle(struct bt_conn *conn, uint16_t handle)
{
	__ASSERT(handle != 0, "Handle cannot be 0");
	__ASSERT(conn, "Conn cannot be NULL");

	for (int i = 0; i < ARRAY_SIZE(vocs_insts); i++) {
		if (vocs_insts[i].cli.conn == conn &&
		    vocs_insts[i].cli.active &&
		    vocs_insts[i].cli.start_handle <= handle &&
		    vocs_insts[i].cli.end_handle >= handle) {
			return &vocs_insts[i];
		}
	}

	BT_DBG("Could not find VOCS instance with handle 0x%04x", handle);
	return NULL;
}

uint8_t vocs_client_notify_handler(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				   const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_vocs *inst;

	if (conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	inst = lookup_vocs_by_handle(conn, handle);

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (!data || !length) {
		return BT_GATT_ITER_CONTINUE;
	}

	if (handle == inst->cli.state_handle) {
		if (length == sizeof(inst->cli.state)) {
			memcpy(&inst->cli.state, data, length);
			BT_DBG("Inst %p: Offset %d, counter %u", inst, inst->cli.state.offset,
			       inst->cli.state.change_counter);
			if (inst->cli.cb && inst->cli.cb->state) {
				inst->cli.cb->state(inst, 0, inst->cli.state.offset);
			}
		} else {
			BT_DBG("Invalid state length %u", length);
		}
	} else if (handle == inst->cli.desc_handle) {
		char desc[MIN(BT_L2CAP_RX_MTU, BT_ATT_MAX_ATTRIBUTE_LEN) + 1];

		/* Truncate if too large */

		if (length > sizeof(desc) - 1) {
			BT_DBG("Description truncated from %u to %zu octets",
			       length, sizeof(desc) - 1);
		}
		length = MIN(sizeof(desc) - 1, length);

		memcpy(desc, data, length);
		desc[length] = '\0';
		BT_DBG("Inst %p: Output description: %s", inst, desc);
		if (inst->cli.cb && inst->cli.cb->description) {
			inst->cli.cb->description(inst, 0, desc);
		}
	} else if (handle == inst->cli.location_handle) {
		if (length == sizeof(inst->cli.location)) {
			memcpy(&inst->cli.location, data, length);
			BT_DBG("Inst %p: Location %u", inst, inst->cli.location);
			if (inst->cli.cb && inst->cli.cb->location) {
				inst->cli.cb->location(inst, 0, inst->cli.location);
			}
		} else {
			BT_DBG("Invalid location length %u", length);
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t vocs_client_read_offset_state_cb(struct bt_conn *conn, uint8_t err,
						struct bt_gatt_read_params *params,
						const void *data, uint16_t length)
{
	int cb_err = err;
	struct bt_vocs *inst = lookup_vocs_by_handle(conn, params->single.handle);

	memset(params, 0, sizeof(*params));

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);
	inst->cli.busy = false;

	if (cb_err) {
		BT_DBG("Offset state read failed: %d", err);
	} else if (data) {
		if (length == sizeof(inst->cli.state)) {
			memcpy(&inst->cli.state, data, length);
			BT_DBG("Offset %d, counter %u",
			       inst->cli.state.offset, inst->cli.state.change_counter);
		} else {
			BT_DBG("Invalid length %u (expected %zu)", length, sizeof(inst->cli.state));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	} else {
		BT_DBG("Invalid state");
		cb_err = BT_ATT_ERR_UNLIKELY;
	}

	if (inst->cli.cb && inst->cli.cb->state) {
		inst->cli.cb->state(inst, cb_err,
				    cb_err ? 0 : inst->cli.state.offset);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t vocs_client_read_location_cb(struct bt_conn *conn, uint8_t err,
					    struct bt_gatt_read_params *params,
					    const void *data, uint16_t length)
{
	int cb_err = err;
	struct bt_vocs *inst = lookup_vocs_by_handle(conn, params->single.handle);

	memset(params, 0, sizeof(*params));

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);
	inst->cli.busy = false;

	if (cb_err) {
		BT_DBG("Offset state read failed: %d", err);
	} else if (data) {
		if (length == sizeof(inst->cli.location)) {
			memcpy(&inst->cli.location, data, length);
			BT_DBG("Location %u", inst->cli.location);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(inst->cli.location));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	} else {
		BT_DBG("Invalid location");
		cb_err = BT_ATT_ERR_UNLIKELY;
	}

	if (inst->cli.cb && inst->cli.cb->location) {
		inst->cli.cb->location(inst, cb_err,
				       cb_err ? 0 : inst->cli.location);
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t internal_read_volume_offset_state_cb(struct bt_conn *conn, uint8_t err,
						    struct bt_gatt_read_params *params,
						    const void *data, uint16_t length)
{
	int cb_err = err;
	struct bt_vocs *inst = lookup_vocs_by_handle(conn, params->single.handle);

	memset(params, 0, sizeof(*params));

	if (!inst) {
		BT_ERR("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	if (err) {
		BT_WARN("Volume offset state read failed: %d", err);
		cb_err = BT_ATT_ERR_UNLIKELY;
	} else if (data) {
		if (length == sizeof(inst->cli.state)) {
			int write_err;

			memcpy(&inst->cli.state, data, length);
			BT_DBG("Offset %d, counter %u",
			       inst->cli.state.offset,
			       inst->cli.state.change_counter);

			/* clear busy flag to reuse function */
			inst->cli.busy = false;
			write_err = bt_vocs_client_state_set(inst, inst->cli.cp.offset);
			if (write_err) {
				cb_err = BT_ATT_ERR_UNLIKELY;
			}
		} else {
			BT_DBG("Invalid length %u (expected %zu)", length, sizeof(inst->cli.state));
			cb_err = BT_ATT_ERR_UNLIKELY;
		}
	} else {
		BT_DBG("Invalid (empty) offset state read");
		cb_err = BT_ATT_ERR_UNLIKELY;
	}

	if (cb_err) {
		inst->cli.busy = false;

		if (inst->cli.cb && inst->cli.cb->set_offset) {
			inst->cli.cb->set_offset(inst, err);
		}
	}

	return BT_GATT_ITER_STOP;
}

static void vcs_client_write_vocs_cp_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_write_params *params)
{
	int cb_err = err;
	struct bt_vocs *inst = lookup_vocs_by_handle(conn, params->handle);

	memset(params, 0, sizeof(*params));

	if (!inst) {
		BT_DBG("Instance not found");
		return;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);

	/* If the change counter is out of data when a write was attempted from the application,
	 * we automatically initiate a read to get the newest state and try again. Once the
	 * change counter has been read, we restart the applications write request. If it fails
	 * the second time, we return an error to the application.
	 */
	if (cb_err == BT_VOCS_ERR_INVALID_COUNTER && inst->cli.cp_retried) {
		cb_err = BT_ATT_ERR_UNLIKELY;
	} else if (cb_err == BT_VOCS_ERR_INVALID_COUNTER && inst->cli.state_handle) {
		BT_DBG("Invalid change counter. Reading volume offset state from server.");

		inst->cli.read_params.func = internal_read_volume_offset_state_cb;
		inst->cli.read_params.handle_count = 1;
		inst->cli.read_params.single.handle = inst->cli.state_handle;

		cb_err = bt_gatt_read(conn, &inst->cli.read_params);
		if (cb_err) {
			BT_WARN("Could not read Volume offset state: %d", cb_err);
		} else {
			inst->cli.cp_retried = true;
			/* Wait for read callback */
			return;
		}
	}

	inst->cli.busy = false;
	inst->cli.cp_retried = false;

	if (inst->cli.cb && inst->cli.cb->set_offset) {
		inst->cli.cb->set_offset(inst, cb_err);
	}
}

static uint8_t vcs_client_read_output_desc_cb(struct bt_conn *conn, uint8_t err,
					      struct bt_gatt_read_params *params,
					      const void *data, uint16_t length)
{
	int cb_err = err;
	struct bt_vocs *inst = lookup_vocs_by_handle(conn, params->single.handle);
	char desc[MIN(BT_L2CAP_RX_MTU, BT_ATT_MAX_ATTRIBUTE_LEN) + 1];

	memset(params, 0, sizeof(*params));

	if (!inst) {
		BT_DBG("Instance not found");
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("Inst %p: err: 0x%02X", inst, err);
	inst->cli.busy = false;

	if (cb_err) {
		BT_DBG("Description read failed: %d", err);
	} else {
		if (data) {
			BT_HEXDUMP_DBG(data, length, "Output description read");

			if (length > sizeof(desc) - 1) {
				BT_DBG("Description truncated from %u to %zu octets",
				       length, sizeof(desc) - 1);
			}
			length = MIN(sizeof(desc) - 1, length);

			/* TODO: Handle long reads */
			memcpy(desc, data, length);
		}
		desc[length] = '\0';
		BT_DBG("Output description: %s", desc);
	}

	if (inst->cli.cb && inst->cli.cb->description) {
		inst->cli.cb->description(inst, cb_err, cb_err ? NULL : desc);
	}

	return BT_GATT_ITER_STOP;
}

static bool valid_inst_discovered(struct bt_vocs *inst)
{
	return inst->cli.state_handle &&
		inst->cli.control_handle &&
		inst->cli.location_handle &&
		inst->cli.desc_handle;
}

static uint8_t vocs_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	struct bt_vocs_client *client_inst = CONTAINER_OF(params,
							  struct bt_vocs_client,
							  discover_params);
	struct bt_vocs *inst = CONTAINER_OF(client_inst, struct bt_vocs, cli);

	if (!attr) {
		BT_DBG("Discovery complete for VOCS %p", inst);
		inst->cli.busy = false;
		(void)memset(params, 0, sizeof(*params));

		if (inst->cli.cb && inst->cli.cb->discover) {
			int err = valid_inst_discovered(inst) ? 0 : -ENOENT;

			inst->cli.cb->discover(inst, err);
		}

		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		struct bt_gatt_subscribe_params *sub_params = NULL;
		struct bt_gatt_chrc *chrc;

		chrc = (struct bt_gatt_chrc *)attr->user_data;
		if (inst->cli.start_handle == 0) {
			inst->cli.start_handle = chrc->value_handle;
		}
		inst->cli.end_handle = chrc->value_handle;

		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_VOCS_STATE)) {
			BT_DBG("Volume offset state");
			inst->cli.state_handle = chrc->value_handle;
			sub_params = &inst->cli.state_sub_params;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_VOCS_LOCATION)) {
			BT_DBG("Location");
			inst->cli.location_handle = chrc->value_handle;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				sub_params = &inst->cli.location_sub_params;
			}
			if (chrc->properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
				inst->cli.location_writable = true;
			}
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_VOCS_CONTROL)) {
			BT_DBG("Control point");
			inst->cli.control_handle = chrc->value_handle;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_VOCS_DESCRIPTION)) {
			BT_DBG("Description");
			inst->cli.desc_handle = chrc->value_handle;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				sub_params = &inst->cli.desc_sub_params;
			}
			if (chrc->properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
				inst->cli.desc_writable = true;
			}
		}

		if (sub_params) {
			int err;

			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = chrc->value_handle;
			/*
			 * TODO: Don't assume that CCC is at handle + 2;
			 * do proper discovery;
			 */
			sub_params->ccc_handle = attr->handle + 2;
			sub_params->notify = vocs_client_notify_handler;
			err = bt_gatt_subscribe(conn, sub_params);
			if (err) {
				BT_WARN("Could not subscribe to handle %u",
					sub_params->ccc_handle);
			}
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

int bt_vocs_client_state_get(struct bt_vocs *inst)
{
	int err;

	CHECKIF(!inst) {
		BT_DBG("NULL instance");
		return -EINVAL;
	}

	CHECKIF(inst->cli.conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	if (!inst->cli.state_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	if (inst->cli.busy) {
		BT_DBG("Handle not set");
		return -EBUSY;
	}

	inst->cli.read_params.func = vocs_client_read_offset_state_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.state_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(inst->cli.conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}

	return err;
}

int bt_vocs_client_location_set(struct bt_vocs *inst, uint32_t location)
{

	CHECKIF(!inst) {
		BT_DBG("NULL instance");
		return -EINVAL;
	}

	CHECKIF(inst->cli.conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	if (!inst->cli.location_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	} else if (!inst->cli.location_writable) {
		BT_DBG("Location is not writable on peer service instance");
		return -EPERM;
	}

	return bt_gatt_write_without_response(inst->cli.conn,
					      inst->cli.location_handle,
					      &location, sizeof(location),
					      false);
}

int bt_vocs_client_location_get(struct bt_vocs *inst)
{
	int err;

	CHECKIF(!inst) {
		BT_DBG("NULL instance");
		return -EINVAL;
	}

	CHECKIF(inst->cli.conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	if (!inst->cli.location_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.read_params.func = vocs_client_read_location_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.location_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(inst->cli.conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}

	return err;
}

int bt_vocs_client_state_set(struct bt_vocs *inst, int16_t offset)
{
	int err;

	CHECKIF(!inst) {
		BT_DBG("NULL instance");
		return -EINVAL;
	}

	CHECKIF(inst->cli.conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	if (!inst->cli.control_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.cp.opcode = BT_VOCS_OPCODE_SET_OFFSET;
	inst->cli.cp.counter = inst->cli.state.change_counter;
	inst->cli.cp.offset = offset;

	inst->cli.write_params.offset = 0;
	inst->cli.write_params.data = &inst->cli.cp;
	inst->cli.write_params.length = sizeof(inst->cli.cp);
	inst->cli.write_params.handle = inst->cli.control_handle;
	inst->cli.write_params.func = vcs_client_write_vocs_cp_cb;

	err = bt_gatt_write(inst->cli.conn, &inst->cli.write_params);
	if (!err) {
		inst->cli.busy = true;
	}

	return err;
}

int bt_vocs_client_description_get(struct bt_vocs *inst)
{
	int err;

	CHECKIF(!inst) {
		BT_DBG("NULL instance");
		return -EINVAL;
	}

	CHECKIF(inst->cli.conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	if (!inst->cli.desc_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	}

	inst->cli.read_params.func = vcs_client_read_output_desc_cb;
	inst->cli.read_params.handle_count = 1;
	inst->cli.read_params.single.handle = inst->cli.desc_handle;
	inst->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(inst->cli.conn, &inst->cli.read_params);
	if (!err) {
		inst->cli.busy = true;
	}

	return err;
}

int bt_vocs_client_description_set(struct bt_vocs *inst,
				   const char *description)
{
	CHECKIF(!inst) {
		BT_DBG("NULL instance");
		return -EINVAL;
	}

	CHECKIF(inst->cli.conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	if (!inst->cli.desc_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (inst->cli.busy) {
		return -EBUSY;
	} else if (!inst->cli.desc_writable) {
		BT_DBG("Description is not writable on peer service instance");
		return -EPERM;
	}

	return bt_gatt_write_without_response(inst->cli.conn,
					      inst->cli.desc_handle,
					      description,
					      strlen(description), false);
}

struct bt_vocs *bt_vocs_client_free_instance_get(void)
{
	for (int i = 0; i < ARRAY_SIZE(vocs_insts); i++) {
		if (!vocs_insts[i].cli.active) {
			vocs_insts[i].client_instance = true;
			vocs_insts[i].cli.active = true;
			return &vocs_insts[i];
		}
	}

	return NULL;
}

int bt_vocs_client_conn_get(const struct bt_vocs *vocs, struct bt_conn **conn)
{
	CHECKIF(vocs == NULL) {
		BT_DBG("NULL vocs pointer");
		return -EINVAL;
	}

	if (!vocs->client_instance) {
		BT_DBG("vocs pointer shall be client instance");
		return -EINVAL;
	}

	if (vocs->cli.conn == NULL) {
		BT_DBG("vocs pointer not associated with a connection. "
		       "Do discovery first");
		return -ENOTCONN;
	}

	*conn = vocs->cli.conn;
	return 0;
}

static void vocs_client_reset(struct bt_vocs *inst)
{
	memset(&inst->cli.state, 0, sizeof(inst->cli.state));
	inst->cli.location_writable = 0;
	inst->cli.location = 0;
	inst->cli.desc_writable = 0;
	inst->cli.start_handle = 0;
	inst->cli.end_handle = 0;
	inst->cli.state_handle = 0;
	inst->cli.location_handle = 0;
	inst->cli.control_handle = 0;
	inst->cli.desc_handle = 0;

	if (inst->cli.conn != NULL) {
		struct bt_conn *conn = inst->cli.conn;

		/* It's okay if these fail. In case of disconnect, we can't
		 * unsubscribe and they will just fail.
		 * In case that we reset due to another call of the discover
		 * function, we will unsubscribe (regardless of bonding state)
		 * to accommodate the new discovery values.
		 */
		(void)bt_gatt_unsubscribe(conn, &inst->cli.state_sub_params);
		(void)bt_gatt_unsubscribe(conn, &inst->cli.location_sub_params);
		(void)bt_gatt_unsubscribe(conn, &inst->cli.desc_sub_params);

		bt_conn_unref(conn);
		inst->cli.conn = NULL;
	}
}

int bt_vocs_discover(struct bt_conn *conn, struct bt_vocs *inst,
		     const struct bt_vocs_discover_param *param)
{
	int err = 0;

	CHECKIF(!inst || !conn || !param) {
		BT_DBG("%s cannot be NULL",
		       inst == NULL ? "inst" : conn == NULL ? "conn" : "param");
		return -EINVAL;
	}

	CHECKIF(param->end_handle < param->start_handle) {
		BT_DBG("start_handle (%u) shall be less than end_handle (%u)",
		       param->start_handle, param->end_handle);
		return -EINVAL;
	}

	CHECKIF(!inst->cli.active) {
		BT_DBG("Inactive instance");
		return -EINVAL;
	}

	if (inst->cli.busy) {
		BT_DBG("Instance is busy");
		return -EBUSY;
	}

	vocs_client_reset(inst);

	inst->cli.conn = bt_conn_ref(conn);
	inst->cli.discover_params.start_handle = param->start_handle;
	inst->cli.discover_params.end_handle = param->end_handle;
	inst->cli.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	inst->cli.discover_params.func = vocs_discover_func;

	err = bt_gatt_discover(conn, &inst->cli.discover_params);
	if (err) {
		BT_DBG("Discover failed (err %d)", err);
	} else {
		inst->cli.busy = true;
	}

	return err;
}

void bt_vocs_client_cb_register(struct bt_vocs *inst, struct bt_vocs_cb *cb)
{
	CHECKIF(!inst) {
		BT_DBG("inst cannot be NULL");
		return;
	}

	inst->cli.cb = cb;
}
