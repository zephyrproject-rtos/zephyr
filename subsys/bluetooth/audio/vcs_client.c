/*  Bluetooth VCS Client - Volume Offset Control Client */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <zephyr/sys/check.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/vcs.h>

#include "vcs_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_VCS_CLIENT)
#define LOG_MODULE_NAME bt_vcs_client
#include "common/log.h"

/* Callback functions */
static struct bt_vcs_cb *vcs_client_cb;

static struct bt_vcs vcs_insts[CONFIG_BT_MAX_CONN];
static int vcs_client_common_vcs_cp(struct bt_vcs *vcs, uint8_t opcode);

bool bt_vcs_client_valid_vocs_inst(struct bt_vcs *vcs, struct bt_vocs *vocs)
{
	if (vcs == NULL) {
		return false;
	}

	if (!vcs->client_instance) {
		return false;
	}

	if (vcs->cli.conn == NULL) {
		return false;
	}

	if (vocs == NULL) {
		return false;
	}

	for (int i = 0; i < ARRAY_SIZE(vcs->cli.vocs); i++) {
		if (vcs->cli.vocs[i] == vocs) {
			return true;
		}
	}

	return false;
}

bool bt_vcs_client_valid_aics_inst(struct bt_vcs *vcs, struct bt_aics *aics)
{
	if (vcs == NULL) {
		return false;
	}

	if (!vcs->client_instance) {
		return false;
	}

	if (vcs->cli.conn == NULL) {
		return false;
	}

	if (aics == NULL) {
		return false;
	}

	for (int i = 0; i < ARRAY_SIZE(vcs->cli.aics); i++) {
		if (vcs->cli.aics[i] == aics) {
			return true;
		}
	}

	return false;
}

static uint8_t vcs_client_notify_handler(struct bt_conn *conn,
					 struct bt_gatt_subscribe_params *params,
					 const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_vcs *vcs_inst;

	if (data == NULL || conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	vcs_inst = &vcs_insts[bt_conn_index(conn)];

	if (handle == vcs_inst->cli.state_handle &&
	    length == sizeof(vcs_inst->cli.state)) {
		memcpy(&vcs_inst->cli.state, data, length);
		BT_DBG("Volume %u, mute %u, counter %u",
			vcs_inst->cli.state.volume, vcs_inst->cli.state.mute,
			vcs_inst->cli.state.change_counter);
		if (vcs_client_cb && vcs_client_cb->state) {
			vcs_client_cb->state(vcs_inst, 0, vcs_inst->cli.state.volume,
					     vcs_inst->cli.state.mute);
		}
	} else if (handle == vcs_inst->cli.flag_handle &&
		   length == sizeof(vcs_inst->cli.flags)) {
		memcpy(&vcs_inst->cli.flags, data, length);
		BT_DBG("Flags %u", vcs_inst->cli.flags);
		if (vcs_client_cb && vcs_client_cb->flags) {
			vcs_client_cb->flags(vcs_inst, 0, vcs_inst->cli.flags);
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t vcs_client_read_vol_state_cb(struct bt_conn *conn, uint8_t err,
					    struct bt_gatt_read_params *params,
					    const void *data, uint16_t length)
{
	int cb_err = err;
	struct bt_vcs *vcs_inst = &vcs_insts[bt_conn_index(conn)];

	vcs_inst->cli.busy = false;

	if (cb_err) {
		BT_DBG("err: %d", cb_err);
	} else if (data != NULL) {
		if (length == sizeof(vcs_inst->cli.state)) {
			memcpy(&vcs_inst->cli.state, data, length);
			BT_DBG("Volume %u, mute %u, counter %u",
			       vcs_inst->cli.state.volume,
			       vcs_inst->cli.state.mute,
			       vcs_inst->cli.state.change_counter);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(vcs_inst->cli.state));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (vcs_client_cb && vcs_client_cb->state) {
		if (cb_err) {
			vcs_client_cb->state(vcs_inst, cb_err, 0, 0);
		} else {
			vcs_client_cb->state(vcs_inst, cb_err,
					     vcs_inst->cli.state.volume,
					     vcs_inst->cli.state.mute);
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t vcs_client_read_flag_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	int cb_err = err;
	struct bt_vcs *vcs_inst = &vcs_insts[bt_conn_index(conn)];

	vcs_inst->cli.busy = false;

	if (cb_err) {
		BT_DBG("err: %d", cb_err);
	} else if (data != NULL) {
		if (length == sizeof(vcs_inst->cli.flags)) {
			memcpy(&vcs_inst->cli.flags, data, length);
			BT_DBG("Flags %u", vcs_inst->cli.flags);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(vcs_inst->cli.flags));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (vcs_client_cb && vcs_client_cb->flags) {
		if (cb_err) {
			vcs_client_cb->flags(vcs_inst, cb_err, 0);
		} else {
			vcs_client_cb->flags(vcs_inst, cb_err, vcs_inst->cli.flags);
		}
	}

	return BT_GATT_ITER_STOP;
}

static void vcs_cp_notify_app(struct bt_vcs *vcs, uint8_t opcode, int err)
{
	if (vcs_client_cb == NULL) {
		return;
	}

	switch (opcode) {
	case BT_VCS_OPCODE_REL_VOL_DOWN:
		if (vcs_client_cb->vol_down) {
			vcs_client_cb->vol_down(vcs, err);
		}
		break;
	case BT_VCS_OPCODE_REL_VOL_UP:
		if (vcs_client_cb->vol_up) {
			vcs_client_cb->vol_up(vcs, err);
		}
		break;
	case BT_VCS_OPCODE_UNMUTE_REL_VOL_DOWN:
		if (vcs_client_cb->vol_down_unmute) {
			vcs_client_cb->vol_down_unmute(vcs, err);
		}
		break;
	case BT_VCS_OPCODE_UNMUTE_REL_VOL_UP:
		if (vcs_client_cb->vol_up_unmute) {
			vcs_client_cb->vol_up_unmute(vcs, err);
		}
		break;
	case BT_VCS_OPCODE_SET_ABS_VOL:
		if (vcs_client_cb->vol_set) {
			vcs_client_cb->vol_set(vcs, err);
		}
		break;
	case BT_VCS_OPCODE_UNMUTE:
		if (vcs_client_cb->unmute) {
			vcs_client_cb->unmute(vcs, err);
		}
		break;
	case BT_VCS_OPCODE_MUTE:
		if (vcs_client_cb->mute) {
			vcs_client_cb->mute(vcs, err);
		}
		break;
	default:
		BT_DBG("Unknown opcode 0x%02x", opcode);
		break;
	}
}

static uint8_t internal_read_vol_state_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	int cb_err = 0;
	struct bt_vcs *vcs_inst = &vcs_insts[bt_conn_index(conn)];
	uint8_t opcode = vcs_inst->cli.cp_val.cp.opcode;


	memset(params, 0, sizeof(*params));

	if (err > 0) {
		BT_WARN("Volume state read failed: %d", err);
		cb_err = BT_ATT_ERR_UNLIKELY;
	} else if (data != NULL) {
		if (length == sizeof(vcs_inst->cli.state)) {
			int write_err;

			memcpy(&vcs_inst->cli.state, data, length);
			BT_DBG("Volume %u, mute %u, counter %u",
			       vcs_inst->cli.state.volume,
			       vcs_inst->cli.state.mute,
			       vcs_inst->cli.state.change_counter);

			/* clear busy flag to reuse function */
			vcs_inst->cli.busy = false;
			if (opcode == BT_VCS_OPCODE_SET_ABS_VOL) {
				write_err = bt_vcs_client_set_volume(vcs_inst,
								     vcs_inst->cli.cp_val.volume);
			} else {
				write_err = vcs_client_common_vcs_cp(vcs_inst,
								     opcode);
			}
			if (write_err) {
				cb_err = BT_ATT_ERR_UNLIKELY;
			}
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(vcs_inst->cli.state));
			cb_err = BT_ATT_ERR_UNLIKELY;
		}
	}

	if (cb_err) {
		vcs_inst->cli.busy = false;
		vcs_cp_notify_app(vcs_inst, opcode, cb_err);
	}

	return BT_GATT_ITER_STOP;
}

static void vcs_client_write_vcs_cp_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_write_params *params)
{
	struct bt_vcs *vcs_inst = &vcs_insts[bt_conn_index(conn)];
	uint8_t opcode = vcs_inst->cli.cp_val.cp.opcode;
	int cb_err = err;

	BT_DBG("err: 0x%02X", err);
	memset(params, 0, sizeof(*params));

	/* If the change counter is out of data when a write was attempted from
	 * the application, we automatically initiate a read to get the newest
	 * state and try again. Once the change counter has been read, we
	 * restart the applications write request. If it fails
	 * the second time, we return an error to the application.
	 */
	if (cb_err == BT_VCS_ERR_INVALID_COUNTER && vcs_inst->cli.cp_retried) {
		cb_err = BT_ATT_ERR_UNLIKELY;
	} else if (err == BT_VCS_ERR_INVALID_COUNTER &&
		   vcs_inst->cli.state_handle) {
		vcs_inst->cli.read_params.func = internal_read_vol_state_cb;
		vcs_inst->cli.read_params.handle_count = 1;
		vcs_inst->cli.read_params.single.handle = vcs_inst->cli.state_handle;
		vcs_inst->cli.read_params.single.offset = 0U;

		cb_err = bt_gatt_read(conn, &vcs_inst->cli.read_params);
		if (cb_err) {
			BT_WARN("Could not read Volume state: %d", cb_err);
		} else {
			vcs_inst->cli.cp_retried = true;
			/* Wait for read callback */
			return;
		}
	}

	vcs_inst->cli.busy = false;
	vcs_inst->cli.cp_retried = false;

	vcs_cp_notify_app(vcs_inst, opcode, err);
}

#if (CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0 || CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0)
static uint8_t vcs_discover_include_func(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 struct bt_gatt_discover_params *params)
{
	struct bt_gatt_include *include;
	uint8_t inst_idx;
	int err;
	struct bt_vcs *vcs_inst = &vcs_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		BT_DBG("Discover include complete for VCS: %u AICS and %u VOCS",
		       vcs_inst->cli.aics_inst_cnt, vcs_inst->cli.vocs_inst_cnt);
		(void)memset(params, 0, sizeof(*params));

		if (vcs_client_cb && vcs_client_cb->discover) {
			/*
			 * TODO: Validate that all mandatory handles were found
			 */
			vcs_client_cb->discover(vcs_inst, 0,
						vcs_inst->cli.vocs_inst_cnt,
						vcs_inst->cli.aics_inst_cnt);
		}

		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		uint8_t conn_index = bt_conn_index(conn);

		include = (struct bt_gatt_include *)attr->user_data;
		BT_DBG("Include UUID %s", bt_uuid_str(include->uuid));

#if CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0
		if (bt_uuid_cmp(include->uuid, BT_UUID_AICS) == 0 &&
		    vcs_inst->cli.aics_inst_cnt < CONFIG_BT_VCS_CLIENT_MAX_AICS_INST) {
			struct bt_aics_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_aics_discover
			 */
			vcs_inst->cli.discover_params.start_handle = attr->handle + 1;

			inst_idx = vcs_inst->cli.aics_inst_cnt++;
			err = bt_aics_discover(conn,
					       vcs_insts[conn_index].cli.aics[inst_idx],
					       &param);
			if (err != 0) {
				BT_DBG("AICS Discover failed (err %d)", err);
				if (vcs_client_cb && vcs_client_cb->discover) {
					vcs_client_cb->discover(vcs_inst, err,
								0, 0);
				}
			}

			return BT_GATT_ITER_STOP;
		}
#endif /* CONFIG_BT_VCS_CLIENT_MAX_AICS_INST */
#if CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0
		if (bt_uuid_cmp(include->uuid, BT_UUID_VOCS) == 0 &&
		    vcs_inst->cli.vocs_inst_cnt < CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST) {
			struct bt_vocs_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_vocs_discover
			 */
			vcs_inst->cli.discover_params.start_handle = attr->handle + 1;

			inst_idx = vcs_inst->cli.vocs_inst_cnt++;
			err = bt_vocs_discover(conn,
					       vcs_insts[conn_index].cli.vocs[inst_idx],
					       &param);
			if (err != 0) {
				BT_DBG("VOCS Discover failed (err %d)", err);
				if (vcs_client_cb && vcs_client_cb->discover) {
					vcs_client_cb->discover(vcs_inst, err,
								0, 0);
				}
			}

			return BT_GATT_ITER_STOP;
		}
#endif /* CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST */
	}

	return BT_GATT_ITER_CONTINUE;
}
#endif /* (CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0 || CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0) */

/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
static uint8_t vcs_discover_func(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 struct bt_gatt_discover_params *params)
{
	int err = 0;
	struct bt_gatt_chrc *chrc;
	struct bt_gatt_subscribe_params *sub_params = NULL;
	struct bt_vcs *vcs_inst = &vcs_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		BT_DBG("Setup complete for VCS");
		(void)memset(params, 0, sizeof(*params));
#if (CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0 || CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0)
		/* Discover included services */
		vcs_inst->cli.discover_params.start_handle = vcs_inst->cli.start_handle;
		vcs_inst->cli.discover_params.end_handle = vcs_inst->cli.end_handle;
		vcs_inst->cli.discover_params.type = BT_GATT_DISCOVER_INCLUDE;
		vcs_inst->cli.discover_params.func = vcs_discover_include_func;

		err = bt_gatt_discover(conn, &vcs_inst->cli.discover_params);
		if (err != 0) {
			BT_DBG("Discover failed (err %d)", err);
			if (vcs_client_cb && vcs_client_cb->discover) {
				vcs_client_cb->discover(vcs_inst, err, 0, 0);
			}
		}
#else
		if (vcs_client_cb && vcs_client_cb->discover) {
			vcs_client_cb->discover(vcs_inst, err, 0, 0);
		}
#endif /* (CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0 || CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0) */

		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_STATE) == 0) {
			BT_DBG("Volume state");
			vcs_inst->cli.state_handle = chrc->value_handle;
			sub_params = &vcs_inst->cli.state_sub_params;
			sub_params->disc_params = &vcs_inst->cli.state_sub_disc_params;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_CONTROL) == 0) {
			BT_DBG("Control Point");
			vcs_inst->cli.control_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_FLAGS) == 0) {
			BT_DBG("Flags");
			vcs_inst->cli.flag_handle = chrc->value_handle;
			sub_params = &vcs_inst->cli.flag_sub_params;
			sub_params->disc_params = &vcs_inst->cli.flag_sub_disc_params;
		}

		if (sub_params != NULL) {
			int err;

			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = chrc->value_handle;
			sub_params->ccc_handle = 0;
			sub_params->end_handle = vcs_inst->cli.end_handle;
			sub_params->notify = vcs_client_notify_handler;
			err = bt_gatt_subscribe(conn, sub_params);
			if (err == 0) {
				BT_DBG("Subscribed to handle 0x%04X",
				       attr->handle);
			} else {
				BT_DBG("Could not subscribe to handle 0x%04X",
				       attr->handle);
			}
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
static uint8_t primary_discover_func(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_service_val *prim_service;
	struct bt_vcs *vcs_inst = &vcs_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		BT_DBG("Could not find a VCS instance on the server");
		if (vcs_client_cb && vcs_client_cb->discover) {
			vcs_client_cb->discover(vcs_inst, -ENODATA, 0, 0);
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		int err;

		BT_DBG("Primary discover complete");
		prim_service = (struct bt_gatt_service_val *)attr->user_data;

		vcs_inst->cli.start_handle = attr->handle + 1;
		vcs_inst->cli.end_handle = prim_service->end_handle;

		/* Discover characteristics */
		vcs_inst->cli.discover_params.uuid = NULL;
		vcs_inst->cli.discover_params.start_handle = vcs_inst->cli.start_handle;
		vcs_inst->cli.discover_params.end_handle = vcs_inst->cli.end_handle;
		vcs_inst->cli.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		vcs_inst->cli.discover_params.func = vcs_discover_func;

		err = bt_gatt_discover(conn, &vcs_inst->cli.discover_params);
		if (err != 0) {
			BT_DBG("Discover failed (err %d)", err);
			if (vcs_client_cb && vcs_client_cb->discover) {
				vcs_client_cb->discover(vcs_inst, err, 0, 0);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int vcs_client_common_vcs_cp(struct bt_vcs *vcs, uint8_t opcode)
{
	int err;

	CHECKIF(vcs->cli.conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	if (vcs->cli.control_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (vcs->cli.busy) {
		return -EBUSY;
	}

	vcs->cli.busy = true;
	vcs->cli.cp_val.cp.opcode = opcode;
	vcs->cli.cp_val.cp.counter = vcs->cli.state.change_counter;
	vcs->cli.write_params.offset = 0;
	vcs->cli.write_params.data = &vcs->cli.cp_val.cp;
	vcs->cli.write_params.length = sizeof(vcs->cli.cp_val.cp);
	vcs->cli.write_params.handle = vcs->cli.control_handle;
	vcs->cli.write_params.func = vcs_client_write_vcs_cp_cb;

	err = bt_gatt_write(vcs->cli.conn, &vcs->cli.write_params);
	if (err == 0) {
		vcs->cli.busy = true;
	}
	return err;
}

#if defined(CONFIG_BT_VCS_CLIENT_AICS)
static struct bt_vcs *lookup_vcs_by_aics(const struct bt_aics *aics)
{
	__ASSERT(aics != NULL, "aics pointer cannot be NULL");

	for (int i = 0; i < ARRAY_SIZE(vcs_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(vcs_insts[i].cli.aics); j++) {
			if (vcs_insts[i].cli.aics[j] == aics) {
				return &vcs_insts[i];
			}
		}
	}

	return NULL;
}

static void aics_discover_cb(struct bt_aics *inst, int err)
{
	struct bt_vcs *vcs_inst = lookup_vcs_by_aics(inst);

	if (err == 0) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(vcs_inst->cli.conn,
				       &vcs_inst->cli.discover_params);
	}

	if (err != 0) {
		BT_DBG("Discover failed (err %d)", err);
		if (vcs_client_cb && vcs_client_cb->discover) {
			vcs_client_cb->discover(vcs_inst, err, 0, 0);
		}
	}
}
#endif /* CONFIG_BT_VCS_CLIENT_AICS */

#if defined(CONFIG_BT_VCS_CLIENT_VOCS)
static struct bt_vcs *lookup_vcs_by_vocs(const struct bt_vocs *vocs)
{
	__ASSERT(vocs != NULL, "VOCS pointer cannot be NULL");

	for (int i = 0; i < ARRAY_SIZE(vcs_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(vcs_insts[i].cli.vocs); j++) {
			if (vcs_insts[i].cli.vocs[j] == vocs) {
				return &vcs_insts[i];
			}
		}
	}

	return NULL;
}

static void vocs_discover_cb(struct bt_vocs *inst, int err)
{
	struct bt_vcs *vcs_inst = lookup_vcs_by_vocs(inst);

	if (vcs_inst == NULL) {
		BT_ERR("Could not lookup vcs_inst from vocs");

		if (vcs_client_cb && vcs_client_cb->discover) {
			vcs_client_cb->discover(vcs_inst,
						BT_GATT_ERR(BT_ATT_ERR_UNLIKELY),
						0, 0);
		}

		return;
	}

	if (err == 0) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(vcs_inst->cli.conn,
				       &vcs_inst->cli.discover_params);
	}

	if (err != 0) {
		BT_DBG("Discover failed (err %d)", err);
		if (vcs_client_cb && vcs_client_cb->discover) {
			vcs_client_cb->discover(vcs_inst, err, 0, 0);
		}
	}
}
#endif /* CONFIG_BT_VCS_CLIENT_VOCS */

static void vcs_client_reset(struct bt_vcs *vcs_inst)
{
	memset(&vcs_inst->cli.state, 0, sizeof(vcs_inst->cli.state));
	vcs_inst->cli.flags = 0;
	vcs_inst->cli.start_handle = 0;
	vcs_inst->cli.end_handle = 0;
	vcs_inst->cli.state_handle = 0;
	vcs_inst->cli.control_handle = 0;
	vcs_inst->cli.flag_handle = 0;
	vcs_inst->cli.vocs_inst_cnt = 0;
	vcs_inst->cli.aics_inst_cnt = 0;

	memset(&vcs_inst->cli.discover_params, 0, sizeof(vcs_inst->cli.discover_params));

	if (vcs_inst->cli.conn != NULL) {
		struct bt_conn *conn = vcs_inst->cli.conn;

		/* It's okay if these fail. In case of disconnect, we can't
		 * unsubscribe and they will just fail.
		 * In case that we reset due to another call of the discover
		 * function, we will unsubscribe (regardless of bonding state)
		 * to accommodate the new discovery values.
		 */
		(void)bt_gatt_unsubscribe(conn, &vcs_inst->cli.state_sub_params);
		(void)bt_gatt_unsubscribe(conn, &vcs_inst->cli.flag_sub_params);

		bt_conn_unref(conn);
		vcs_inst->cli.conn = NULL;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_vcs *vcs_inst = &vcs_insts[bt_conn_index(conn)];

	if (vcs_inst->cli.conn == conn) {
		vcs_client_reset(vcs_inst);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

static void bt_vcs_client_init(void)
{
	int i, j;

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) &&
	    CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0) {
		for (i = 0; i < ARRAY_SIZE(vcs_insts); i++) {
			for (j = 0; j < ARRAY_SIZE(vcs_insts[i].cli.vocs); j++) {
				vcs_insts[i].cli.vocs[j] = bt_vocs_client_free_instance_get();

				__ASSERT(vcs_insts[i].cli.vocs[j],
					 "Could not allocate VOCS client instance");

				bt_vocs_client_cb_register(vcs_insts[i].cli.vocs[j],
							   &vcs_client_cb->vocs_cb);
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) &&
	    CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0) {
		for (i = 0; i < ARRAY_SIZE(vcs_insts); i++) {
			for (j = 0; j < ARRAY_SIZE(vcs_insts[i].cli.aics); j++) {
				vcs_insts[i].cli.aics[j] = bt_aics_client_free_instance_get();

				__ASSERT(vcs_insts[i].cli.aics[j],
					"Could not allocate AICS client instance");

				bt_aics_client_cb_register(vcs_insts[i].cli.aics[j],
							   &vcs_client_cb->aics_cb);
			}
		}
	}
}

int bt_vcs_discover(struct bt_conn *conn, struct bt_vcs **vcs)
{
	static bool initialized;
	struct bt_vcs *vcs_inst;
	int err;

	/*
	 * This will initiate a discover procedure. The procedure will do the
	 * following sequence:
	 * 1) Primary discover for the VCS
	 * 2) Characteristic discover of the VCS
	 * 3) Discover services included in VCS (VOCS and AICS)
	 * 4) For each included service found; discovery of the characteristics
	 * 5) When everything above have been discovered, the callback is called
	 */

	CHECKIF(conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	vcs_inst = &vcs_insts[bt_conn_index(conn)];

	if (vcs_inst->cli.busy) {
		return -EBUSY;
	}

	if (!initialized) {
		bt_vcs_client_init();
		initialized = true;
	}

	vcs_client_reset(vcs_inst);

	memcpy(&vcs_inst->cli.uuid, BT_UUID_VCS, sizeof(vcs_inst->cli.uuid));

	vcs_inst->client_instance = true;
	vcs_inst->cli.conn = bt_conn_ref(conn);
	vcs_inst->cli.discover_params.func = primary_discover_func;
	vcs_inst->cli.discover_params.uuid = &vcs_inst->cli.uuid.uuid;
	vcs_inst->cli.discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	vcs_inst->cli.discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	vcs_inst->cli.discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	err = bt_gatt_discover(conn, &vcs_inst->cli.discover_params);
	if (err == 0) {
		*vcs = vcs_inst;
	}
	return err;
}

int bt_vcs_client_cb_register(struct bt_vcs_cb *cb)
{
#if defined(CONFIG_BT_VCS_CLIENT_VOCS)
	struct bt_vocs_cb *vocs_cb = NULL;

	if (cb != NULL) {
		/* Ensure that the cb->vocs_cb.discover is the vocs_discover_cb */
		CHECKIF(cb->vocs_cb.discover != NULL &&
			cb->vocs_cb.discover != vocs_discover_cb) {
			BT_ERR("VOCS discover callback shall not be set");
			return -EINVAL;
		}
		cb->vocs_cb.discover = vocs_discover_cb;

		vocs_cb = &cb->vocs_cb;
	}

	for (int i = 0; i < ARRAY_SIZE(vcs_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(vcs_insts[i].cli.vocs); j++) {
			struct bt_vocs *vocs = vcs_insts[i].cli.vocs[j];

			if (vocs != NULL) {
				bt_vocs_client_cb_register(vocs, vocs_cb);
			}
		}
	}
#endif /* CONFIG_BT_VCS_CLIENT_VOCS */

#if defined(CONFIG_BT_VCS_CLIENT_AICS)
	struct bt_aics_cb *aics_cb = NULL;

	if (cb != NULL) {
		/* Ensure that the cb->aics_cb.discover is the aics_discover_cb */
		CHECKIF(cb->aics_cb.discover != NULL &&
			cb->aics_cb.discover != aics_discover_cb) {
			BT_ERR("AICS discover callback shall not be set");
			return -EINVAL;
		}
		cb->aics_cb.discover = aics_discover_cb;

		aics_cb = &cb->aics_cb;
	}

	for (int i = 0; i < ARRAY_SIZE(vcs_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(vcs_insts[i].cli.aics); j++) {
			struct bt_aics *aics = vcs_insts[i].cli.aics[j];

			if (aics != NULL) {
				bt_aics_client_cb_register(aics, aics_cb);
			}
		}
	}
#endif /* CONFIG_BT_VCS_CLIENT_AICS */

	vcs_client_cb = cb;

	return 0;
}

int bt_vcs_client_included_get(struct bt_vcs *vcs,
			       struct bt_vcs_included *included)
{
	CHECKIF(!included || vcs == NULL) {
		return -EINVAL;
	}

	included->vocs_cnt = vcs->cli.vocs_inst_cnt;
	included->vocs = vcs->cli.vocs;

	included->aics_cnt = vcs->cli.aics_inst_cnt;
	included->aics = vcs->cli.aics;

	return 0;
}

int bt_vcs_client_conn_get(const struct bt_vcs *vcs, struct bt_conn **conn)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs pointer");
		return -EINVAL;
	}

	if (!vcs->client_instance) {
		BT_DBG("vcs pointer shall be client instance");
		return -EINVAL;
	}

	if (vcs->cli.conn == NULL) {
		BT_DBG("vcs pointer not associated with a connection. "
		       "Do discovery first");
		return -ENOTCONN;
	}

	*conn = vcs->cli.conn;
	return 0;
}

int bt_vcs_client_read_vol_state(struct bt_vcs *vcs)
{
	int err;

	CHECKIF(vcs->cli.conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	if (vcs->cli.state_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (vcs->cli.busy) {
		return -EBUSY;
	}

	vcs->cli.read_params.func = vcs_client_read_vol_state_cb;
	vcs->cli.read_params.handle_count = 1;
	vcs->cli.read_params.single.handle = vcs->cli.state_handle;
	vcs->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(vcs->cli.conn, &vcs->cli.read_params);
	if (err == 0) {
		vcs->cli.busy = true;
	}

	return err;
}

int bt_vcs_client_read_flags(struct bt_vcs *vcs)
{
	int err;

	CHECKIF(vcs->cli.conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	if (vcs->cli.flag_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (vcs->cli.busy) {
		return -EBUSY;
	}

	vcs->cli.read_params.func = vcs_client_read_flag_cb;
	vcs->cli.read_params.handle_count = 1;
	vcs->cli.read_params.single.handle = vcs->cli.flag_handle;
	vcs->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(vcs->cli.conn, &vcs->cli.read_params);
	if (err == 0) {
		vcs->cli.busy = true;
	}

	return err;
}

int bt_vcs_client_vol_down(struct bt_vcs *vcs)
{
	return vcs_client_common_vcs_cp(vcs, BT_VCS_OPCODE_REL_VOL_DOWN);
}

int bt_vcs_client_vol_up(struct bt_vcs *vcs)
{
	return vcs_client_common_vcs_cp(vcs, BT_VCS_OPCODE_REL_VOL_UP);
}

int bt_vcs_client_unmute_vol_down(struct bt_vcs *vcs)
{
	return vcs_client_common_vcs_cp(vcs, BT_VCS_OPCODE_UNMUTE_REL_VOL_DOWN);
}

int bt_vcs_client_unmute_vol_up(struct bt_vcs *vcs)
{
	return vcs_client_common_vcs_cp(vcs, BT_VCS_OPCODE_UNMUTE_REL_VOL_UP);
}

int bt_vcs_client_set_volume(struct bt_vcs *vcs, uint8_t volume)
{
	int err;

	CHECKIF(vcs->cli.conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	if (vcs->cli.control_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (vcs->cli.busy) {
		return -EBUSY;
	}

	vcs->cli.cp_val.cp.opcode = BT_VCS_OPCODE_SET_ABS_VOL;
	vcs->cli.cp_val.cp.counter = vcs->cli.state.change_counter;
	vcs->cli.cp_val.volume = volume;

	vcs->cli.busy = true;
	vcs->cli.write_params.offset = 0;
	vcs->cli.write_params.data = &vcs->cli.cp_val;
	vcs->cli.write_params.length = sizeof(vcs->cli.cp_val);
	vcs->cli.write_params.handle = vcs->cli.control_handle;
	vcs->cli.write_params.func = vcs_client_write_vcs_cp_cb;

	err = bt_gatt_write(vcs->cli.conn, &vcs->cli.write_params);
	if (err == 0) {
		vcs->cli.busy = true;
	}

	return err;
}

int bt_vcs_client_unmute(struct bt_vcs *vcs)
{
	return vcs_client_common_vcs_cp(vcs, BT_VCS_OPCODE_UNMUTE);
}

int bt_vcs_client_mute(struct bt_vcs *vcs)
{
	return vcs_client_common_vcs_cp(vcs, BT_VCS_OPCODE_MUTE);
}
