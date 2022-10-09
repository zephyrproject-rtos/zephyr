/*  Bluetooth VCP Client - Volume Offset Control Client */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/audio/vcp.h>

#include "vcp_internal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_vcp_client, CONFIG_BT_VCP_CLIENT_LOG_LEVEL);

#include "common/bt_str.h"

/* Callback functions */
static struct bt_vcp_cb *vcp_client_cb;

static struct bt_vcp vcp_insts[CONFIG_BT_MAX_CONN];
static int vcp_client_common_vcs_cp(struct bt_vcp *vcp, uint8_t opcode);

bool bt_vcp_client_valid_vocs_inst(struct bt_vcp *vcp, struct bt_vocs *vocs)
{
	if (vcp == NULL) {
		return false;
	}

	if (!vcp->client_instance) {
		return false;
	}

	if (vcp->cli.conn == NULL) {
		return false;
	}

	if (vocs == NULL) {
		return false;
	}

	for (int i = 0; i < ARRAY_SIZE(vcp->cli.vocs); i++) {
		if (vcp->cli.vocs[i] == vocs) {
			return true;
		}
	}

	return false;
}

bool bt_vcp_client_valid_aics_inst(struct bt_vcp *vcp, struct bt_aics *aics)
{
	if (vcp == NULL) {
		return false;
	}

	if (!vcp->client_instance) {
		return false;
	}

	if (vcp->cli.conn == NULL) {
		return false;
	}

	if (aics == NULL) {
		return false;
	}

	for (int i = 0; i < ARRAY_SIZE(vcp->cli.aics); i++) {
		if (vcp->cli.aics[i] == aics) {
			return true;
		}
	}

	return false;
}

static uint8_t vcp_client_notify_handler(struct bt_conn *conn,
					 struct bt_gatt_subscribe_params *params,
					 const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_vcp *vcp_inst;

	if (data == NULL || conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	vcp_inst = &vcp_insts[bt_conn_index(conn)];

	if (handle == vcp_inst->cli.state_handle &&
	    length == sizeof(vcp_inst->cli.state)) {
		memcpy(&vcp_inst->cli.state, data, length);
		LOG_DBG("Volume %u, mute %u, counter %u",
			vcp_inst->cli.state.volume, vcp_inst->cli.state.mute,
			vcp_inst->cli.state.change_counter);
		if (vcp_client_cb && vcp_client_cb->state) {
			vcp_client_cb->state(vcp_inst, 0, vcp_inst->cli.state.volume,
					     vcp_inst->cli.state.mute);
		}
	} else if (handle == vcp_inst->cli.flag_handle &&
		   length == sizeof(vcp_inst->cli.flags)) {
		memcpy(&vcp_inst->cli.flags, data, length);
		LOG_DBG("Flags %u", vcp_inst->cli.flags);
		if (vcp_client_cb && vcp_client_cb->flags) {
			vcp_client_cb->flags(vcp_inst, 0, vcp_inst->cli.flags);
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t vcp_client_read_vol_state_cb(struct bt_conn *conn, uint8_t err,
					    struct bt_gatt_read_params *params,
					    const void *data, uint16_t length)
{
	int cb_err = err;
	struct bt_vcp *vcp_inst = &vcp_insts[bt_conn_index(conn)];

	vcp_inst->cli.busy = false;

	if (cb_err) {
		LOG_DBG("err: %d", cb_err);
	} else if (data != NULL) {
		if (length == sizeof(vcp_inst->cli.state)) {
			memcpy(&vcp_inst->cli.state, data, length);
			LOG_DBG("Volume %u, mute %u, counter %u",
				vcp_inst->cli.state.volume,
				vcp_inst->cli.state.mute,
				vcp_inst->cli.state.change_counter);
		} else {
			LOG_DBG("Invalid length %u (expected %zu)",
				length, sizeof(vcp_inst->cli.state));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (vcp_client_cb && vcp_client_cb->state) {
		if (cb_err) {
			vcp_client_cb->state(vcp_inst, cb_err, 0, 0);
		} else {
			vcp_client_cb->state(vcp_inst, cb_err,
					     vcp_inst->cli.state.volume,
					     vcp_inst->cli.state.mute);
		}
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t vcp_client_read_flag_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	int cb_err = err;
	struct bt_vcp *vcp_inst = &vcp_insts[bt_conn_index(conn)];

	vcp_inst->cli.busy = false;

	if (cb_err) {
		LOG_DBG("err: %d", cb_err);
	} else if (data != NULL) {
		if (length == sizeof(vcp_inst->cli.flags)) {
			memcpy(&vcp_inst->cli.flags, data, length);
			LOG_DBG("Flags %u", vcp_inst->cli.flags);
		} else {
			LOG_DBG("Invalid length %u (expected %zu)",
				length, sizeof(vcp_inst->cli.flags));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (vcp_client_cb && vcp_client_cb->flags) {
		if (cb_err) {
			vcp_client_cb->flags(vcp_inst, cb_err, 0);
		} else {
			vcp_client_cb->flags(vcp_inst, cb_err, vcp_inst->cli.flags);
		}
	}

	return BT_GATT_ITER_STOP;
}

static void vcs_cp_notify_app(struct bt_vcp *vcp, uint8_t opcode, int err)
{
	if (vcp_client_cb == NULL) {
		return;
	}

	switch (opcode) {
	case BT_VCP_OPCODE_REL_VOL_DOWN:
		if (vcp_client_cb->vol_down) {
			vcp_client_cb->vol_down(vcp, err);
		}
		break;
	case BT_VCP_OPCODE_REL_VOL_UP:
		if (vcp_client_cb->vol_up) {
			vcp_client_cb->vol_up(vcp, err);
		}
		break;
	case BT_VCP_OPCODE_UNMUTE_REL_VOL_DOWN:
		if (vcp_client_cb->vol_down_unmute) {
			vcp_client_cb->vol_down_unmute(vcp, err);
		}
		break;
	case BT_VCP_OPCODE_UNMUTE_REL_VOL_UP:
		if (vcp_client_cb->vol_up_unmute) {
			vcp_client_cb->vol_up_unmute(vcp, err);
		}
		break;
	case BT_VCP_OPCODE_SET_ABS_VOL:
		if (vcp_client_cb->vol_set) {
			vcp_client_cb->vol_set(vcp, err);
		}
		break;
	case BT_VCP_OPCODE_UNMUTE:
		if (vcp_client_cb->unmute) {
			vcp_client_cb->unmute(vcp, err);
		}
		break;
	case BT_VCP_OPCODE_MUTE:
		if (vcp_client_cb->mute) {
			vcp_client_cb->mute(vcp, err);
		}
		break;
	default:
		LOG_DBG("Unknown opcode 0x%02x", opcode);
		break;
	}
}

static uint8_t internal_read_vol_state_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	int cb_err = 0;
	struct bt_vcp *vcp_inst = &vcp_insts[bt_conn_index(conn)];
	uint8_t opcode = vcp_inst->cli.cp_val.cp.opcode;


	memset(params, 0, sizeof(*params));

	if (err > 0) {
		LOG_WRN("Volume state read failed: %d", err);
		cb_err = BT_ATT_ERR_UNLIKELY;
	} else if (data != NULL) {
		if (length == sizeof(vcp_inst->cli.state)) {
			int write_err;

			memcpy(&vcp_inst->cli.state, data, length);
			LOG_DBG("Volume %u, mute %u, counter %u",
				vcp_inst->cli.state.volume,
				vcp_inst->cli.state.mute,
				vcp_inst->cli.state.change_counter);

			/* clear busy flag to reuse function */
			vcp_inst->cli.busy = false;
			if (opcode == BT_VCP_OPCODE_SET_ABS_VOL) {
				write_err = bt_vcp_client_set_volume(vcp_inst,
								     vcp_inst->cli.cp_val.volume);
			} else {
				write_err = vcp_client_common_vcs_cp(vcp_inst,
								     opcode);
			}
			if (write_err) {
				cb_err = BT_ATT_ERR_UNLIKELY;
			}
		} else {
			LOG_DBG("Invalid length %u (expected %zu)",
				length, sizeof(vcp_inst->cli.state));
			cb_err = BT_ATT_ERR_UNLIKELY;
		}
	}

	if (cb_err) {
		vcp_inst->cli.busy = false;
		vcs_cp_notify_app(vcp_inst, opcode, cb_err);
	}

	return BT_GATT_ITER_STOP;
}

static void vcp_client_write_vcs_cp_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_write_params *params)
{
	struct bt_vcp *vcp_inst = &vcp_insts[bt_conn_index(conn)];
	uint8_t opcode = vcp_inst->cli.cp_val.cp.opcode;
	int cb_err = err;

	LOG_DBG("err: 0x%02X", err);
	memset(params, 0, sizeof(*params));

	/* If the change counter is out of data when a write was attempted from
	 * the application, we automatically initiate a read to get the newest
	 * state and try again. Once the change counter has been read, we
	 * restart the applications write request. If it fails
	 * the second time, we return an error to the application.
	 */
	if (cb_err == BT_VCP_ERR_INVALID_COUNTER && vcp_inst->cli.cp_retried) {
		cb_err = BT_ATT_ERR_UNLIKELY;
	} else if (err == BT_VCP_ERR_INVALID_COUNTER &&
		   vcp_inst->cli.state_handle) {
		vcp_inst->cli.read_params.func = internal_read_vol_state_cb;
		vcp_inst->cli.read_params.handle_count = 1;
		vcp_inst->cli.read_params.single.handle = vcp_inst->cli.state_handle;
		vcp_inst->cli.read_params.single.offset = 0U;

		cb_err = bt_gatt_read(conn, &vcp_inst->cli.read_params);
		if (cb_err) {
			LOG_WRN("Could not read Volume state: %d", cb_err);
		} else {
			vcp_inst->cli.cp_retried = true;
			/* Wait for read callback */
			return;
		}
	}

	vcp_inst->cli.busy = false;
	vcp_inst->cli.cp_retried = false;

	vcs_cp_notify_app(vcp_inst, opcode, err);
}

#if (CONFIG_BT_VCP_CLIENT_MAX_AICS_INST > 0 || CONFIG_BT_VCP_CLIENT_MAX_VOCS_INST > 0)
static uint8_t vcs_discover_include_func(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 struct bt_gatt_discover_params *params)
{
	struct bt_gatt_include *include;
	uint8_t inst_idx;
	int err;
	struct bt_vcp *vcp_inst = &vcp_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		LOG_DBG("Discover include complete for VCP: %u AICS and %u VOCS",
			vcp_inst->cli.aics_inst_cnt,
			vcp_inst->cli.vocs_inst_cnt);
		(void)memset(params, 0, sizeof(*params));

		if (vcp_client_cb && vcp_client_cb->discover) {
			/*
			 * TODO: Validate that all mandatory handles were found
			 */
			vcp_client_cb->discover(vcp_inst, 0,
						vcp_inst->cli.vocs_inst_cnt,
						vcp_inst->cli.aics_inst_cnt);
		}

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		uint8_t conn_index = bt_conn_index(conn);

		include = (struct bt_gatt_include *)attr->user_data;
		LOG_DBG("Include UUID %s", bt_uuid_str(include->uuid));

#if CONFIG_BT_VCP_CLIENT_MAX_AICS_INST > 0
		if (bt_uuid_cmp(include->uuid, BT_UUID_AICS) == 0 &&
		    vcp_inst->cli.aics_inst_cnt < CONFIG_BT_VCP_CLIENT_MAX_AICS_INST) {
			struct bt_aics_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_aics_discover
			 */
			vcp_inst->cli.discover_params.start_handle = attr->handle + 1;

			inst_idx = vcp_inst->cli.aics_inst_cnt++;
			err = bt_aics_discover(conn,
					       vcp_insts[conn_index].cli.aics[inst_idx],
					       &param);
			if (err != 0) {
				LOG_DBG("AICS Discover failed (err %d)", err);
				if (vcp_client_cb && vcp_client_cb->discover) {
					vcp_client_cb->discover(vcp_inst, err,
								0, 0);
				}
			}

			return BT_GATT_ITER_STOP;
		}
#endif /* CONFIG_BT_VCP_CLIENT_MAX_AICS_INST */
#if CONFIG_BT_VCP_CLIENT_MAX_VOCS_INST > 0
		if (bt_uuid_cmp(include->uuid, BT_UUID_VOCS) == 0 &&
		    vcp_inst->cli.vocs_inst_cnt < CONFIG_BT_VCP_CLIENT_MAX_VOCS_INST) {
			struct bt_vocs_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_vocs_discover
			 */
			vcp_inst->cli.discover_params.start_handle = attr->handle + 1;

			inst_idx = vcp_inst->cli.vocs_inst_cnt++;
			err = bt_vocs_discover(conn,
					       vcp_insts[conn_index].cli.vocs[inst_idx],
					       &param);
			if (err != 0) {
				LOG_DBG("VOCS Discover failed (err %d)", err);
				if (vcp_client_cb && vcp_client_cb->discover) {
					vcp_client_cb->discover(vcp_inst, err,
								0, 0);
				}
			}

			return BT_GATT_ITER_STOP;
		}
#endif /* CONFIG_BT_VCP_CLIENT_MAX_VOCS_INST */
	}

	return BT_GATT_ITER_CONTINUE;
}
#endif /* (CONFIG_BT_VCP_CLIENT_MAX_AICS_INST > 0 || CONFIG_BT_VCP_CLIENT_MAX_VOCS_INST > 0) */

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
	struct bt_vcp *vcp_inst = &vcp_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		LOG_DBG("Setup complete for VCP");
		(void)memset(params, 0, sizeof(*params));
#if (CONFIG_BT_VCP_CLIENT_MAX_AICS_INST > 0 || CONFIG_BT_VCP_CLIENT_MAX_VOCS_INST > 0)
		/* Discover included services */
		vcp_inst->cli.discover_params.start_handle = vcp_inst->cli.start_handle;
		vcp_inst->cli.discover_params.end_handle = vcp_inst->cli.end_handle;
		vcp_inst->cli.discover_params.type = BT_GATT_DISCOVER_INCLUDE;
		vcp_inst->cli.discover_params.func = vcs_discover_include_func;

		err = bt_gatt_discover(conn, &vcp_inst->cli.discover_params);
		if (err != 0) {
			LOG_DBG("Discover failed (err %d)", err);
			if (vcp_client_cb && vcp_client_cb->discover) {
				vcp_client_cb->discover(vcp_inst, err, 0, 0);
			}
		}
#else
		if (vcp_client_cb && vcp_client_cb->discover) {
			vcp_client_cb->discover(vcp_inst, err, 0, 0);
		}
#endif /* (CONFIG_BT_VCP_CLIENT_MAX_AICS_INST > 0 || CONFIG_BT_VCP_CLIENT_MAX_VOCS_INST > 0) */

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_STATE) == 0) {
			LOG_DBG("Volume state");
			vcp_inst->cli.state_handle = chrc->value_handle;
			sub_params = &vcp_inst->cli.state_sub_params;
			sub_params->disc_params = &vcp_inst->cli.state_sub_disc_params;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_CONTROL) == 0) {
			LOG_DBG("Control Point");
			vcp_inst->cli.control_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_FLAGS) == 0) {
			LOG_DBG("Flags");
			vcp_inst->cli.flag_handle = chrc->value_handle;
			sub_params = &vcp_inst->cli.flag_sub_params;
			sub_params->disc_params = &vcp_inst->cli.flag_sub_disc_params;
		}

		if (sub_params != NULL) {
			int err;

			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = chrc->value_handle;
			sub_params->ccc_handle = 0;
			sub_params->end_handle = vcp_inst->cli.end_handle;
			sub_params->notify = vcp_client_notify_handler;
			err = bt_gatt_subscribe(conn, sub_params);
			if (err == 0) {
				LOG_DBG("Subscribed to handle 0x%04X",
					attr->handle);
			} else {
				LOG_DBG("Could not subscribe to handle 0x%04X",
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
	struct bt_vcp *vcp_inst = &vcp_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		LOG_DBG("Could not find a VCP instance on the server");
		if (vcp_client_cb && vcp_client_cb->discover) {
			vcp_client_cb->discover(vcp_inst, -ENODATA, 0, 0);
		}
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		int err;

		LOG_DBG("Primary discover complete");
		prim_service = (struct bt_gatt_service_val *)attr->user_data;

		vcp_inst->cli.start_handle = attr->handle + 1;
		vcp_inst->cli.end_handle = prim_service->end_handle;

		/* Discover characteristics */
		vcp_inst->cli.discover_params.uuid = NULL;
		vcp_inst->cli.discover_params.start_handle = vcp_inst->cli.start_handle;
		vcp_inst->cli.discover_params.end_handle = vcp_inst->cli.end_handle;
		vcp_inst->cli.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		vcp_inst->cli.discover_params.func = vcs_discover_func;

		err = bt_gatt_discover(conn, &vcp_inst->cli.discover_params);
		if (err != 0) {
			LOG_DBG("Discover failed (err %d)", err);
			if (vcp_client_cb && vcp_client_cb->discover) {
				vcp_client_cb->discover(vcp_inst, err, 0, 0);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int vcp_client_common_vcs_cp(struct bt_vcp *vcp, uint8_t opcode)
{
	int err;

	CHECKIF(vcp->cli.conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	if (vcp->cli.control_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	} else if (vcp->cli.busy) {
		return -EBUSY;
	}

	vcp->cli.busy = true;
	vcp->cli.cp_val.cp.opcode = opcode;
	vcp->cli.cp_val.cp.counter = vcp->cli.state.change_counter;
	vcp->cli.write_params.offset = 0;
	vcp->cli.write_params.data = &vcp->cli.cp_val.cp;
	vcp->cli.write_params.length = sizeof(vcp->cli.cp_val.cp);
	vcp->cli.write_params.handle = vcp->cli.control_handle;
	vcp->cli.write_params.func = vcp_client_write_vcs_cp_cb;

	err = bt_gatt_write(vcp->cli.conn, &vcp->cli.write_params);
	if (err == 0) {
		vcp->cli.busy = true;
	}
	return err;
}

#if defined(CONFIG_BT_VCP_CLIENT_AICS)
static struct bt_vcp *lookup_vcp_by_aics(const struct bt_aics *aics)
{
	__ASSERT(aics != NULL, "aics pointer cannot be NULL");

	for (int i = 0; i < ARRAY_SIZE(vcp_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(vcp_insts[i].cli.aics); j++) {
			if (vcp_insts[i].cli.aics[j] == aics) {
				return &vcp_insts[i];
			}
		}
	}

	return NULL;
}

static void aics_discover_cb(struct bt_aics *inst, int err)
{
	struct bt_vcp *vcp_inst = lookup_vcp_by_aics(inst);

	if (err == 0) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(vcp_inst->cli.conn,
				       &vcp_inst->cli.discover_params);
	}

	if (err != 0) {
		LOG_DBG("Discover failed (err %d)", err);
		if (vcp_client_cb && vcp_client_cb->discover) {
			vcp_client_cb->discover(vcp_inst, err, 0, 0);
		}
	}
}
#endif /* CONFIG_BT_VCP_CLIENT_AICS */

#if defined(CONFIG_BT_VCP_CLIENT_VOCS)
static struct bt_vcp *lookup_vcp_by_vocs(const struct bt_vocs *vocs)
{
	__ASSERT(vocs != NULL, "VOCS pointer cannot be NULL");

	for (int i = 0; i < ARRAY_SIZE(vcp_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(vcp_insts[i].cli.vocs); j++) {
			if (vcp_insts[i].cli.vocs[j] == vocs) {
				return &vcp_insts[i];
			}
		}
	}

	return NULL;
}

static void vocs_discover_cb(struct bt_vocs *inst, int err)
{
	struct bt_vcp *vcp_inst = lookup_vcp_by_vocs(inst);

	if (vcp_inst == NULL) {
		LOG_ERR("Could not lookup vcp_inst from vocs");

		if (vcp_client_cb && vcp_client_cb->discover) {
			vcp_client_cb->discover(vcp_inst,
						BT_GATT_ERR(BT_ATT_ERR_UNLIKELY),
						0, 0);
		}

		return;
	}

	if (err == 0) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(vcp_inst->cli.conn,
				       &vcp_inst->cli.discover_params);
	}

	if (err != 0) {
		LOG_DBG("Discover failed (err %d)", err);
		if (vcp_client_cb && vcp_client_cb->discover) {
			vcp_client_cb->discover(vcp_inst, err, 0, 0);
		}
	}
}
#endif /* CONFIG_BT_VCP_CLIENT_VOCS */

static void vcp_client_reset(struct bt_vcp *vcp_inst)
{
	memset(&vcp_inst->cli.state, 0, sizeof(vcp_inst->cli.state));
	vcp_inst->cli.flags = 0;
	vcp_inst->cli.start_handle = 0;
	vcp_inst->cli.end_handle = 0;
	vcp_inst->cli.state_handle = 0;
	vcp_inst->cli.control_handle = 0;
	vcp_inst->cli.flag_handle = 0;
	vcp_inst->cli.vocs_inst_cnt = 0;
	vcp_inst->cli.aics_inst_cnt = 0;

	memset(&vcp_inst->cli.discover_params, 0, sizeof(vcp_inst->cli.discover_params));

	if (vcp_inst->cli.conn != NULL) {
		struct bt_conn *conn = vcp_inst->cli.conn;

		/* It's okay if these fail. In case of disconnect, we can't
		 * unsubscribe and they will just fail.
		 * In case that we reset due to another call of the discover
		 * function, we will unsubscribe (regardless of bonding state)
		 * to accommodate the new discovery values.
		 */
		(void)bt_gatt_unsubscribe(conn, &vcp_inst->cli.state_sub_params);
		(void)bt_gatt_unsubscribe(conn, &vcp_inst->cli.flag_sub_params);

		bt_conn_unref(conn);
		vcp_inst->cli.conn = NULL;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_vcp *vcp_inst = &vcp_insts[bt_conn_index(conn)];

	if (vcp_inst->cli.conn == conn) {
		vcp_client_reset(vcp_inst);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

static void bt_vcp_client_init(void)
{
	int i, j;

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) &&
	    CONFIG_BT_VCP_CLIENT_MAX_VOCS_INST > 0) {
		for (i = 0; i < ARRAY_SIZE(vcp_insts); i++) {
			for (j = 0; j < ARRAY_SIZE(vcp_insts[i].cli.vocs); j++) {
				vcp_insts[i].cli.vocs[j] = bt_vocs_client_free_instance_get();

				__ASSERT(vcp_insts[i].cli.vocs[j],
					 "Could not allocate VOCS client instance");

				bt_vocs_client_cb_register(vcp_insts[i].cli.vocs[j],
							   &vcp_client_cb->vocs_cb);
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) &&
	    CONFIG_BT_VCP_CLIENT_MAX_AICS_INST > 0) {
		for (i = 0; i < ARRAY_SIZE(vcp_insts); i++) {
			for (j = 0; j < ARRAY_SIZE(vcp_insts[i].cli.aics); j++) {
				vcp_insts[i].cli.aics[j] = bt_aics_client_free_instance_get();

				__ASSERT(vcp_insts[i].cli.aics[j],
					"Could not allocate AICS client instance");

				bt_aics_client_cb_register(vcp_insts[i].cli.aics[j],
							   &vcp_client_cb->aics_cb);
			}
		}
	}
}

int bt_vcp_discover(struct bt_conn *conn, struct bt_vcp **vcp)
{
	static bool initialized;
	struct bt_vcp *vcp_inst;
	int err;

	/*
	 * This will initiate a discover procedure. The procedure will do the
	 * following sequence:
	 * 1) Primary discover for the VCP
	 * 2) Characteristic discover of the VCP
	 * 3) Discover services included in VCP (VOCS and AICS)
	 * 4) For each included service found; discovery of the characteristics
	 * 5) When everything above have been discovered, the callback is called
	 */

	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	vcp_inst = &vcp_insts[bt_conn_index(conn)];

	if (vcp_inst->cli.busy) {
		return -EBUSY;
	}

	if (!initialized) {
		bt_vcp_client_init();
		initialized = true;
	}

	vcp_client_reset(vcp_inst);

	memcpy(&vcp_inst->cli.uuid, BT_UUID_VCS, sizeof(vcp_inst->cli.uuid));

	vcp_inst->client_instance = true;
	vcp_inst->cli.conn = bt_conn_ref(conn);
	vcp_inst->cli.discover_params.func = primary_discover_func;
	vcp_inst->cli.discover_params.uuid = &vcp_inst->cli.uuid.uuid;
	vcp_inst->cli.discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	vcp_inst->cli.discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	vcp_inst->cli.discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	err = bt_gatt_discover(conn, &vcp_inst->cli.discover_params);
	if (err == 0) {
		*vcp = vcp_inst;
	}
	return err;
}

int bt_vcp_client_cb_register(struct bt_vcp_cb *cb)
{
#if defined(CONFIG_BT_VCP_CLIENT_VOCS)
	struct bt_vocs_cb *vocs_cb = NULL;

	if (cb != NULL) {
		/* Ensure that the cb->vocs_cb.discover is the vocs_discover_cb */
		CHECKIF(cb->vocs_cb.discover != NULL &&
			cb->vocs_cb.discover != vocs_discover_cb) {
			LOG_ERR("VOCS discover callback shall not be set");
			return -EINVAL;
		}
		cb->vocs_cb.discover = vocs_discover_cb;

		vocs_cb = &cb->vocs_cb;
	}

	for (int i = 0; i < ARRAY_SIZE(vcp_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(vcp_insts[i].cli.vocs); j++) {
			struct bt_vocs *vocs = vcp_insts[i].cli.vocs[j];

			if (vocs != NULL) {
				bt_vocs_client_cb_register(vocs, vocs_cb);
			}
		}
	}
#endif /* CONFIG_BT_VCP_CLIENT_VOCS */

#if defined(CONFIG_BT_VCP_CLIENT_AICS)
	struct bt_aics_cb *aics_cb = NULL;

	if (cb != NULL) {
		/* Ensure that the cb->aics_cb.discover is the aics_discover_cb */
		CHECKIF(cb->aics_cb.discover != NULL &&
			cb->aics_cb.discover != aics_discover_cb) {
			LOG_ERR("AICS discover callback shall not be set");
			return -EINVAL;
		}
		cb->aics_cb.discover = aics_discover_cb;

		aics_cb = &cb->aics_cb;
	}

	for (int i = 0; i < ARRAY_SIZE(vcp_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(vcp_insts[i].cli.aics); j++) {
			struct bt_aics *aics = vcp_insts[i].cli.aics[j];

			if (aics != NULL) {
				bt_aics_client_cb_register(aics, aics_cb);
			}
		}
	}
#endif /* CONFIG_BT_VCP_CLIENT_AICS */

	vcp_client_cb = cb;

	return 0;
}

int bt_vcp_client_included_get(struct bt_vcp *vcp,
			       struct bt_vcp_included *included)
{
	CHECKIF(!included || vcp == NULL) {
		return -EINVAL;
	}

	included->vocs_cnt = vcp->cli.vocs_inst_cnt;
	included->vocs = vcp->cli.vocs;

	included->aics_cnt = vcp->cli.aics_inst_cnt;
	included->aics = vcp->cli.aics;

	return 0;
}

int bt_vcp_client_conn_get(const struct bt_vcp *vcp, struct bt_conn **conn)
{
	CHECKIF(vcp == NULL) {
		LOG_DBG("NULL vcp pointer");
		return -EINVAL;
	}

	if (!vcp->client_instance) {
		LOG_DBG("vcp pointer shall be client instance");
		return -EINVAL;
	}

	if (vcp->cli.conn == NULL) {
		LOG_DBG("vcp pointer not associated with a connection. "
		       "Do discovery first");
		return -ENOTCONN;
	}

	*conn = vcp->cli.conn;
	return 0;
}

int bt_vcp_client_read_vol_state(struct bt_vcp *vcp)
{
	int err;

	CHECKIF(vcp->cli.conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	if (vcp->cli.state_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	} else if (vcp->cli.busy) {
		return -EBUSY;
	}

	vcp->cli.read_params.func = vcp_client_read_vol_state_cb;
	vcp->cli.read_params.handle_count = 1;
	vcp->cli.read_params.single.handle = vcp->cli.state_handle;
	vcp->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(vcp->cli.conn, &vcp->cli.read_params);
	if (err == 0) {
		vcp->cli.busy = true;
	}

	return err;
}

int bt_vcp_client_read_flags(struct bt_vcp *vcp)
{
	int err;

	CHECKIF(vcp->cli.conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	if (vcp->cli.flag_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	} else if (vcp->cli.busy) {
		return -EBUSY;
	}

	vcp->cli.read_params.func = vcp_client_read_flag_cb;
	vcp->cli.read_params.handle_count = 1;
	vcp->cli.read_params.single.handle = vcp->cli.flag_handle;
	vcp->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(vcp->cli.conn, &vcp->cli.read_params);
	if (err == 0) {
		vcp->cli.busy = true;
	}

	return err;
}

int bt_vcp_client_vol_down(struct bt_vcp *vcp)
{
	return vcp_client_common_vcs_cp(vcp, BT_VCP_OPCODE_REL_VOL_DOWN);
}

int bt_vcp_client_vol_up(struct bt_vcp *vcp)
{
	return vcp_client_common_vcs_cp(vcp, BT_VCP_OPCODE_REL_VOL_UP);
}

int bt_vcp_client_unmute_vol_down(struct bt_vcp *vcp)
{
	return vcp_client_common_vcs_cp(vcp, BT_VCP_OPCODE_UNMUTE_REL_VOL_DOWN);
}

int bt_vcp_client_unmute_vol_up(struct bt_vcp *vcp)
{
	return vcp_client_common_vcs_cp(vcp, BT_VCP_OPCODE_UNMUTE_REL_VOL_UP);
}

int bt_vcp_client_set_volume(struct bt_vcp *vcp, uint8_t volume)
{
	int err;

	CHECKIF(vcp->cli.conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	if (vcp->cli.control_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	} else if (vcp->cli.busy) {
		return -EBUSY;
	}

	vcp->cli.cp_val.cp.opcode = BT_VCP_OPCODE_SET_ABS_VOL;
	vcp->cli.cp_val.cp.counter = vcp->cli.state.change_counter;
	vcp->cli.cp_val.volume = volume;

	vcp->cli.busy = true;
	vcp->cli.write_params.offset = 0;
	vcp->cli.write_params.data = &vcp->cli.cp_val;
	vcp->cli.write_params.length = sizeof(vcp->cli.cp_val);
	vcp->cli.write_params.handle = vcp->cli.control_handle;
	vcp->cli.write_params.func = vcp_client_write_vcs_cp_cb;

	err = bt_gatt_write(vcp->cli.conn, &vcp->cli.write_params);
	if (err == 0) {
		vcp->cli.busy = true;
	}

	return err;
}

int bt_vcp_client_unmute(struct bt_vcp *vcp)
{
	return vcp_client_common_vcs_cp(vcp, BT_VCP_OPCODE_UNMUTE);
}

int bt_vcp_client_mute(struct bt_vcp *vcp)
{
	return vcp_client_common_vcs_cp(vcp, BT_VCP_OPCODE_MUTE);
}
