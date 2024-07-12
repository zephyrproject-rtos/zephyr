/*  Bluetooth vol_ctlr Client - Volume Offset Control Client */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/audio/vocs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "common/bt_str.h"
#include "vcp_internal.h"

LOG_MODULE_REGISTER(bt_vcp_vol_ctlr, CONFIG_BT_VCP_VOL_CTLR_LOG_LEVEL);

/* Callback functions */
static sys_slist_t vcp_vol_ctlr_cbs = SYS_SLIST_STATIC_INIT(&vcp_vol_ctlr_cbs);

static struct bt_vcp_vol_ctlr vol_ctlr_insts[CONFIG_BT_MAX_CONN];
static int vcp_vol_ctlr_common_vcs_cp(struct bt_vcp_vol_ctlr *vol_ctlr, uint8_t opcode);

static struct bt_vcp_vol_ctlr *vol_ctlr_get_by_conn(const struct bt_conn *conn)
{
	return &vol_ctlr_insts[bt_conn_index(conn)];
}

static void vcp_vol_ctlr_state_changed(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->state) {
			listener->state(vol_ctlr, err, err == 0 ? vol_ctlr->state.volume : 0,
					err == 0 ? vol_ctlr->state.mute : 0);
		}
	}
}

static void vcp_vol_ctlr_flags_changed(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->flags) {
			listener->flags(vol_ctlr, err, err == 0 ? vol_ctlr->flags : 0);
		}
	}
}

static void vcp_vol_ctlr_discover_complete(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->discover) {
			uint8_t vocs_cnt = 0U;
			uint8_t aics_cnt = 0U;

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
			if (err == 0) {
				vocs_cnt = vol_ctlr->vocs_inst_cnt;
			}
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#if defined(CONFIG_BT_VCP_VOL_CTLR_AICS)
			if (err == 0) {
				aics_cnt = vol_ctlr->aics_inst_cnt;
			}
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */

			listener->discover(vol_ctlr, err, vocs_cnt, aics_cnt);
		}
	}
}

static uint8_t vcp_vol_ctlr_notify_handler(struct bt_conn *conn,
					 struct bt_gatt_subscribe_params *params,
					 const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_vcp_vol_ctlr *vol_ctlr;

	if (data == NULL || conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	vol_ctlr = vol_ctlr_get_by_conn(conn);

	if (handle == vol_ctlr->state_handle &&
	    length == sizeof(vol_ctlr->state)) {
		memcpy(&vol_ctlr->state, data, length);
		LOG_DBG("Volume %u, mute %u, counter %u",
			vol_ctlr->state.volume, vol_ctlr->state.mute,
			vol_ctlr->state.change_counter);
		vcp_vol_ctlr_state_changed(vol_ctlr, 0);
	} else if (handle == vol_ctlr->flag_handle &&
		   length == sizeof(vol_ctlr->flags)) {
		memcpy(&vol_ctlr->flags, data, length);
		LOG_DBG("Flags %u", vol_ctlr->flags);
		vcp_vol_ctlr_flags_changed(vol_ctlr, 0);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t vcp_vol_ctlr_read_vol_state_cb(struct bt_conn *conn, uint8_t err,
					      struct bt_gatt_read_params *params,
					      const void *data, uint16_t length)
{
	int cb_err = err;
	struct bt_vcp_vol_ctlr *vol_ctlr = vol_ctlr_get_by_conn(conn);

	vol_ctlr->busy = false;

	if (cb_err) {
		LOG_DBG("err: %d", cb_err);
	} else if (data != NULL) {
		if (length == sizeof(vol_ctlr->state)) {
			memcpy(&vol_ctlr->state, data, length);
			LOG_DBG("Volume %u, mute %u, counter %u",
				vol_ctlr->state.volume,
				vol_ctlr->state.mute,
				vol_ctlr->state.change_counter);
		} else {
			LOG_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(vol_ctlr->state));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	vcp_vol_ctlr_state_changed(vol_ctlr, cb_err);

	return BT_GATT_ITER_STOP;
}

static uint8_t vcp_vol_ctlr_read_flag_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	int cb_err = err;
	struct bt_vcp_vol_ctlr *vol_ctlr = vol_ctlr_get_by_conn(conn);

	vol_ctlr->busy = false;

	if (cb_err) {
		LOG_DBG("err: %d", cb_err);
	} else if (data != NULL) {
		if (length == sizeof(vol_ctlr->flags)) {
			memcpy(&vol_ctlr->flags, data, length);
			LOG_DBG("Flags %u", vol_ctlr->flags);
		} else {
			LOG_DBG("Invalid length %u (expected %zu)",
				length, sizeof(vol_ctlr->flags));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	vcp_vol_ctlr_flags_changed(vol_ctlr, cb_err);

	return BT_GATT_ITER_STOP;
}

static void vcs_cp_notify_app(struct bt_vcp_vol_ctlr *vol_ctlr, uint8_t opcode, int err)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	LOG_DBG("%p opcode %u err %d", vol_ctlr, opcode, err);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		switch (opcode) {
		case BT_VCP_OPCODE_REL_VOL_DOWN:
			if (listener->vol_down) {
				listener->vol_down(vol_ctlr, err);
			}
			break;
		case BT_VCP_OPCODE_REL_VOL_UP:
			if (listener->vol_up) {
				listener->vol_up(vol_ctlr, err);
			}
			break;
		case BT_VCP_OPCODE_UNMUTE_REL_VOL_DOWN:
			if (listener->vol_down_unmute) {
				listener->vol_down_unmute(vol_ctlr, err);
			}
			break;
		case BT_VCP_OPCODE_UNMUTE_REL_VOL_UP:
			if (listener->vol_up_unmute) {
				listener->vol_up_unmute(vol_ctlr, err);
			}
			break;
		case BT_VCP_OPCODE_SET_ABS_VOL:
			if (listener->vol_set) {
				listener->vol_set(vol_ctlr, err);
			}
			break;
		case BT_VCP_OPCODE_UNMUTE:
			if (listener->unmute) {
				listener->unmute(vol_ctlr, err);
			}
			break;
		case BT_VCP_OPCODE_MUTE:
			if (listener->mute) {
				listener->mute(vol_ctlr, err);
			}
			break;
		default:
			LOG_DBG("Unknown opcode 0x%02x", opcode);
			break;
		}
	}
}

static uint8_t internal_read_vol_state_cb(struct bt_conn *conn, uint8_t err,
					  struct bt_gatt_read_params *params,
					  const void *data, uint16_t length)
{
	int cb_err = 0;
	struct bt_vcp_vol_ctlr *vol_ctlr = vol_ctlr_get_by_conn(conn);
	uint8_t opcode = vol_ctlr->cp_val.cp.opcode;


	memset(params, 0, sizeof(*params));

	if (err > 0) {
		LOG_WRN("Volume state read failed: %d", err);
		cb_err = BT_ATT_ERR_UNLIKELY;
	} else if (data != NULL) {
		if (length == sizeof(vol_ctlr->state)) {
			int write_err;

			memcpy(&vol_ctlr->state, data, length);
			LOG_DBG("Volume %u, mute %u, counter %u",
				vol_ctlr->state.volume,
				vol_ctlr->state.mute,
				vol_ctlr->state.change_counter);

			/* clear busy flag to reuse function */
			vol_ctlr->busy = false;
			if (opcode == BT_VCP_OPCODE_SET_ABS_VOL) {
				write_err = bt_vcp_vol_ctlr_set_vol(vol_ctlr,
								     vol_ctlr->cp_val.volume);
			} else {
				write_err = vcp_vol_ctlr_common_vcs_cp(vol_ctlr,
								     opcode);
			}
			if (write_err) {
				cb_err = BT_ATT_ERR_UNLIKELY;
			}
		} else {
			LOG_DBG("Invalid length %u (expected %zu)",
				length, sizeof(vol_ctlr->state));
			cb_err = BT_ATT_ERR_UNLIKELY;
		}
	}

	if (cb_err) {
		vol_ctlr->busy = false;
		vcs_cp_notify_app(vol_ctlr, opcode, cb_err);
	}

	return BT_GATT_ITER_STOP;
}

static void vcp_vol_ctlr_write_vcs_cp_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_write_params *params)
{
	struct bt_vcp_vol_ctlr *vol_ctlr = vol_ctlr_get_by_conn(conn);
	uint8_t opcode = vol_ctlr->cp_val.cp.opcode;
	int cb_err = err;

	LOG_DBG("err: 0x%02X", err);
	memset(params, 0, sizeof(*params));

	/* If the change counter is out of data when a write was attempted from
	 * the application, we automatically initiate a read to get the newest
	 * state and try again. Once the change counter has been read, we
	 * restart the applications write request. If it fails
	 * the second time, we return an error to the application.
	 */
	if (cb_err == BT_VCP_ERR_INVALID_COUNTER && vol_ctlr->cp_retried) {
		cb_err = BT_ATT_ERR_UNLIKELY;
	} else if (err == BT_VCP_ERR_INVALID_COUNTER &&
		   vol_ctlr->state_handle) {
		vol_ctlr->read_params.func = internal_read_vol_state_cb;
		vol_ctlr->read_params.handle_count = 1;
		vol_ctlr->read_params.single.handle = vol_ctlr->state_handle;
		vol_ctlr->read_params.single.offset = 0U;

		cb_err = bt_gatt_read(conn, &vol_ctlr->read_params);
		if (cb_err) {
			LOG_WRN("Could not read Volume state: %d", cb_err);
		} else {
			vol_ctlr->cp_retried = true;
			/* Wait for read callback */
			return;
		}
	}

	vol_ctlr->busy = false;
	vol_ctlr->cp_retried = false;

	vcs_cp_notify_app(vol_ctlr, opcode, err);
}

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS) || defined(CONFIG_BT_VCP_VOL_CTLR_AICS)
static uint8_t vcs_discover_include_func(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 struct bt_gatt_discover_params *params)
{
	struct bt_gatt_include *include;
	uint8_t inst_idx;
	int err;
	struct bt_vcp_vol_ctlr *vol_ctlr = vol_ctlr_get_by_conn(conn);

	if (attr == NULL) {
		uint8_t vocs_cnt = 0U;
		uint8_t aics_cnt = 0U;

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
		vocs_cnt = vol_ctlr->vocs_inst_cnt;
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */

#if defined(CONFIG_BT_VCP_VOL_CTLR_AICS)
		aics_cnt = vol_ctlr->aics_inst_cnt;
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */

		LOG_DBG("Discover include complete for vol_ctlr: %u AICS and %u VOCS", aics_cnt,
			vocs_cnt);
		(void)memset(params, 0, sizeof(*params));

		vcp_vol_ctlr_discover_complete(vol_ctlr, 0);

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		uint8_t conn_index = bt_conn_index(conn);

		include = (struct bt_gatt_include *)attr->user_data;
		LOG_DBG("Include UUID %s", bt_uuid_str(include->uuid));

#if defined(CONFIG_BT_VCP_VOL_CTLR_AICS)
		if (bt_uuid_cmp(include->uuid, BT_UUID_AICS) == 0 &&
		    vol_ctlr->aics_inst_cnt < CONFIG_BT_VCP_VOL_CTLR_MAX_AICS_INST) {
			struct bt_aics_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_aics_discover
			 */
			vol_ctlr->discover_params.start_handle = attr->handle + 1;

			inst_idx = vol_ctlr->aics_inst_cnt++;
			err = bt_aics_discover(conn,
					       vol_ctlr_insts[conn_index].aics[inst_idx],
					       &param);
			if (err != 0) {
				LOG_DBG("AICS Discover failed (err %d)", err);
				vcp_vol_ctlr_discover_complete(vol_ctlr, err);
			}

			return BT_GATT_ITER_STOP;
		}
#endif /* CONFIG_BT_VCP_VOL_CTLR_AICS */
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
		if (bt_uuid_cmp(include->uuid, BT_UUID_VOCS) == 0 &&
		    vol_ctlr->vocs_inst_cnt < CONFIG_BT_VCP_VOL_CTLR_MAX_VOCS_INST) {
			struct bt_vocs_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_vocs_discover
			 */
			vol_ctlr->discover_params.start_handle = attr->handle + 1;

			inst_idx = vol_ctlr->vocs_inst_cnt++;
			err = bt_vocs_discover(conn,
					       vol_ctlr_insts[conn_index].vocs[inst_idx],
					       &param);
			if (err != 0) {
				LOG_DBG("VOCS Discover failed (err %d)", err);
				vcp_vol_ctlr_discover_complete(vol_ctlr, err);
			}

			return BT_GATT_ITER_STOP;
		}
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
	}

	return BT_GATT_ITER_CONTINUE;
}
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS || CONFIG_BT_VCP_VOL_CTLR_AICS */

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
	struct bt_vcp_vol_ctlr *vol_ctlr = vol_ctlr_get_by_conn(conn);

	if (attr == NULL) {
		LOG_DBG("Setup complete for vol_ctlr");
		(void)memset(params, 0, sizeof(*params));
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS) || defined(CONFIG_BT_VCP_VOL_CTLR_AICS)
		/* Discover included services */
		vol_ctlr->discover_params.start_handle = vol_ctlr->start_handle;
		vol_ctlr->discover_params.end_handle = vol_ctlr->end_handle;
		vol_ctlr->discover_params.type = BT_GATT_DISCOVER_INCLUDE;
		vol_ctlr->discover_params.func = vcs_discover_include_func;

		err = bt_gatt_discover(conn, &vol_ctlr->discover_params);
		if (err != 0) {
			LOG_DBG("Discover failed (err %d)", err);
			vcp_vol_ctlr_discover_complete(vol_ctlr, err);
		}
#else
		vcp_vol_ctlr_discover_complete(vol_ctlr, err);
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS || CONFIG_BT_VCP_VOL_CTLR_AICS */

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_STATE) == 0) {
			LOG_DBG("Volume state");
			vol_ctlr->state_handle = bt_gatt_attr_value_handle(attr);
			sub_params = &vol_ctlr->state_sub_params;
			sub_params->disc_params = &vol_ctlr->state_sub_disc_params;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_CONTROL) == 0) {
			LOG_DBG("Control Point");
			vol_ctlr->control_handle = bt_gatt_attr_value_handle(attr);
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_VCS_FLAGS) == 0) {
			LOG_DBG("Flags");
			vol_ctlr->flag_handle = bt_gatt_attr_value_handle(attr);
			sub_params = &vol_ctlr->flag_sub_params;
			sub_params->disc_params = &vol_ctlr->flag_sub_disc_params;
		}

		if (sub_params != NULL) {
			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = bt_gatt_attr_value_handle(attr);
			sub_params->ccc_handle = 0;
			sub_params->end_handle = vol_ctlr->end_handle;
			sub_params->notify = vcp_vol_ctlr_notify_handler;
			atomic_set_bit(sub_params->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

			err = bt_gatt_subscribe(conn, sub_params);
			if (err == 0 || err == -EALREADY) {
				LOG_DBG("Subscribed to handle 0x%04X",
					attr->handle);
			} else {
				LOG_DBG("Could not subscribe to handle 0x%04X (%d)", attr->handle,
					err);
				vcp_vol_ctlr_discover_complete(vol_ctlr, err);

				return BT_GATT_ITER_STOP;
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
	struct bt_vcp_vol_ctlr *vol_ctlr = vol_ctlr_get_by_conn(conn);

	if (attr == NULL) {
		LOG_DBG("Could not find a vol_ctlr instance on the server");
		vcp_vol_ctlr_discover_complete(vol_ctlr, -ENODATA);

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		int err;

		LOG_DBG("Primary discover complete");
		prim_service = (struct bt_gatt_service_val *)attr->user_data;

		vol_ctlr->start_handle = attr->handle + 1;
		vol_ctlr->end_handle = prim_service->end_handle;

		/* Discover characteristics */
		vol_ctlr->discover_params.uuid = NULL;
		vol_ctlr->discover_params.start_handle = vol_ctlr->start_handle;
		vol_ctlr->discover_params.end_handle = vol_ctlr->end_handle;
		vol_ctlr->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		vol_ctlr->discover_params.func = vcs_discover_func;

		err = bt_gatt_discover(conn, &vol_ctlr->discover_params);
		if (err != 0) {
			LOG_DBG("Discover failed (err %d)", err);
			vcp_vol_ctlr_discover_complete(vol_ctlr, err);
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int vcp_vol_ctlr_common_vcs_cp(struct bt_vcp_vol_ctlr *vol_ctlr, uint8_t opcode)
{
	int err;

	CHECKIF(vol_ctlr == NULL) {
		LOG_DBG("NULL ctlr");
		return -EINVAL;
	}

	if (vol_ctlr->conn == NULL) {
		LOG_DBG("NULL ctlr conn");
		return -EINVAL;
	}

	if (vol_ctlr->control_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	} else if (vol_ctlr->busy) {
		return -EBUSY;
	}

	vol_ctlr->busy = true;
	vol_ctlr->cp_val.cp.opcode = opcode;
	vol_ctlr->cp_val.cp.counter = vol_ctlr->state.change_counter;
	vol_ctlr->write_params.offset = 0;
	vol_ctlr->write_params.data = &vol_ctlr->cp_val.cp;
	vol_ctlr->write_params.length = sizeof(vol_ctlr->cp_val.cp);
	vol_ctlr->write_params.handle = vol_ctlr->control_handle;
	vol_ctlr->write_params.func = vcp_vol_ctlr_write_vcs_cp_cb;

	err = bt_gatt_write(vol_ctlr->conn, &vol_ctlr->write_params);
	if (err == 0) {
		vol_ctlr->busy = true;
	}
	return err;
}

#if defined(CONFIG_BT_VCP_VOL_CTLR_AICS)
static struct bt_vcp_vol_ctlr *lookup_vcp_by_aics(const struct bt_aics *aics)
{
	__ASSERT(aics != NULL, "aics pointer cannot be NULL");

	for (size_t i = 0U; i < ARRAY_SIZE(vol_ctlr_insts); i++) {
		for (size_t j = 0U; j < ARRAY_SIZE(vol_ctlr_insts[i].aics); j++) {
			if (vol_ctlr_insts[i].aics[j] == aics) {
				return &vol_ctlr_insts[i];
			}
		}
	}

	return NULL;
}

static void vcp_vol_ctlr_aics_state_cb(struct bt_aics *inst, int err, int8_t gain, uint8_t mute,
				       uint8_t mode)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.state) {
			listener->aics_cb.state(inst, err, gain, mute, mode);
		}
	}
}

static void vcp_vol_ctlr_aics_gain_setting_cb(struct bt_aics *inst, int err, uint8_t units,
					      int8_t minimum, int8_t maximum)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.gain_setting) {
			listener->aics_cb.gain_setting(inst, err, units, minimum, maximum);
		}
	}
}

static void vcp_vol_ctlr_aics_type_cb(struct bt_aics *inst, int err, uint8_t type)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.type) {
			listener->aics_cb.type(inst, err, type);
		}
	}
}

static void vcp_vol_ctlr_aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.status) {
			listener->aics_cb.status(inst, err, active);
		}
	}
}

static void vcp_vol_ctlr_aics_description_cb(struct bt_aics *inst, int err, char *description)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.description) {
			listener->aics_cb.description(inst, err, description);
		}
	}
}

static void vcp_vol_ctlr_aics_discover_cb(struct bt_aics *inst, int err)
{
	struct bt_vcp_vol_ctlr *vol_ctlr = lookup_vcp_by_aics(inst);
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	if (vol_ctlr == NULL) {
		LOG_ERR("Could not lookup vol_ctlr from aics");
		vcp_vol_ctlr_discover_complete(vol_ctlr, BT_GATT_ERR(BT_ATT_ERR_UNLIKELY));

		return;
	}

	if (err == 0) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(vol_ctlr->conn, &vol_ctlr->discover_params);
	}

	if (err != 0) {
		LOG_DBG("Discover failed (err %d)", err);
		vcp_vol_ctlr_discover_complete(vol_ctlr, err);
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.discover) {
			listener->aics_cb.discover(inst, err);
		}
	}
}

static void vcp_vol_ctlr_aics_set_gain_cb(struct bt_aics *inst, int err)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.set_gain) {
			listener->aics_cb.set_gain(inst, err);
		}
	}
}

static void vcp_vol_ctlr_aics_unmute_cb(struct bt_aics *inst, int err)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.unmute) {
			listener->aics_cb.unmute(inst, err);
		}
	}
}

static void vcp_vol_ctlr_aics_mute_cb(struct bt_aics *inst, int err)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.mute) {
			listener->aics_cb.mute(inst, err);
		}
	}
}

static void vcp_vol_ctlr_aics_set_manual_mode_cb(struct bt_aics *inst, int err)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.set_manual_mode) {
			listener->aics_cb.set_manual_mode(inst, err);
		}
	}
}

static void vcp_vol_ctlr_aics_set_auto_mode_cb(struct bt_aics *inst, int err)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.set_auto_mode) {
			listener->aics_cb.set_auto_mode(inst, err);
		}
	}
}
#endif /* CONFIG_BT_VCP_VOL_CTLR_AICS */

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
static struct bt_vcp_vol_ctlr *lookup_vcp_by_vocs(const struct bt_vocs *vocs)
{
	__ASSERT(vocs != NULL, "VOCS pointer cannot be NULL");

	for (int i = 0; i < ARRAY_SIZE(vol_ctlr_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(vol_ctlr_insts[i].vocs); j++) {
			if (vol_ctlr_insts[i].vocs[j] == vocs) {
				return &vol_ctlr_insts[i];
			}
		}
	}

	return NULL;
}

static void vcp_vol_ctlr_vocs_state_cb(struct bt_vocs *inst, int err, int16_t offset)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->vocs_cb.state) {
			listener->vocs_cb.state(inst, err, offset);
		}
	}
}

static void vcp_vol_ctlr_vocs_location_cb(struct bt_vocs *inst, int err, uint32_t location)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->vocs_cb.location) {
			listener->vocs_cb.location(inst, err, location);
		}
	}
}

static void vcp_vol_ctlr_vocs_description_cb(struct bt_vocs *inst, int err, char *description)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->vocs_cb.description) {
			listener->vocs_cb.description(inst, err, description);
		}
	}
}

static void vcp_vol_ctlr_vocs_discover_cb(struct bt_vocs *inst, int err)
{
	struct bt_vcp_vol_ctlr *vol_ctlr = lookup_vcp_by_vocs(inst);
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	if (vol_ctlr == NULL) {
		LOG_ERR("Could not lookup vol_ctlr from vocs");
		vcp_vol_ctlr_discover_complete(vol_ctlr, BT_GATT_ERR(BT_ATT_ERR_UNLIKELY));

		return;
	}

	if (err == 0) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(vol_ctlr->conn, &vol_ctlr->discover_params);
	}

	if (err != 0) {
		LOG_DBG("Discover failed (err %d)", err);
		vcp_vol_ctlr_discover_complete(vol_ctlr, err);
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->vocs_cb.discover) {
			listener->vocs_cb.discover(inst, err);
		}
	}
}

static void vcp_vol_ctlr_vocs_set_offset_cb(struct bt_vocs *inst, int err)
{
	struct bt_vcp_vol_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&vcp_vol_ctlr_cbs, listener, next, _node) {
		if (listener->vocs_cb.set_offset) {
			listener->vocs_cb.set_offset(inst, err);
		}
	}
}
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */

static void vcp_vol_ctlr_reset(struct bt_vcp_vol_ctlr *vol_ctlr)
{
	memset(&vol_ctlr->state, 0, sizeof(vol_ctlr->state));
	vol_ctlr->flags = 0;
	vol_ctlr->start_handle = 0;
	vol_ctlr->end_handle = 0;
	vol_ctlr->state_handle = 0;
	vol_ctlr->control_handle = 0;
	vol_ctlr->flag_handle = 0;
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
	vol_ctlr->vocs_inst_cnt = 0;
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#if defined(CONFIG_BT_VCP_VOL_CTLR_AICS)
	vol_ctlr->aics_inst_cnt = 0;
#endif /* CONFIG_BT_VCP_VOL_CTLR_AICS */

	memset(&vol_ctlr->discover_params, 0, sizeof(vol_ctlr->discover_params));

	if (vol_ctlr->conn != NULL) {
		struct bt_conn *conn = vol_ctlr->conn;

		bt_conn_unref(conn);
		vol_ctlr->conn = NULL;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_vcp_vol_ctlr *vol_ctlr = vol_ctlr_get_by_conn(conn);

	if (vol_ctlr->conn == conn) {
		vcp_vol_ctlr_reset(vol_ctlr);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

static void bt_vcp_vol_ctlr_init(void)
{
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
	for (size_t i = 0U; i < ARRAY_SIZE(vol_ctlr_insts); i++) {
		for (size_t j = 0U; j < ARRAY_SIZE(vol_ctlr_insts[i].vocs); j++) {
			static struct bt_vocs_cb vocs_cb = {
				.state = vcp_vol_ctlr_vocs_state_cb,
				.location = vcp_vol_ctlr_vocs_location_cb,
				.description = vcp_vol_ctlr_vocs_description_cb,
				.discover = vcp_vol_ctlr_vocs_discover_cb,
				.set_offset = vcp_vol_ctlr_vocs_set_offset_cb,
			};

			vol_ctlr_insts[i].vocs[j] = bt_vocs_client_free_instance_get();

			__ASSERT(vol_ctlr_insts[i].vocs[j],
				 "Could not allocate VOCS client instance");

			bt_vocs_client_cb_register(vol_ctlr_insts[i].vocs[j], &vocs_cb);
		}
	}
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */

#if defined(CONFIG_BT_VCP_VOL_CTLR_AICS)
	for (size_t i = 0U; i < ARRAY_SIZE(vol_ctlr_insts); i++) {
		for (size_t j = 0U; j < ARRAY_SIZE(vol_ctlr_insts[i].aics); j++) {
			static struct bt_aics_cb aics_cb = {
				.state = vcp_vol_ctlr_aics_state_cb,
				.gain_setting = vcp_vol_ctlr_aics_gain_setting_cb,
				.type = vcp_vol_ctlr_aics_type_cb,
				.status = vcp_vol_ctlr_aics_status_cb,
				.description = vcp_vol_ctlr_aics_description_cb,
				.discover = vcp_vol_ctlr_aics_discover_cb,
				.set_gain = vcp_vol_ctlr_aics_set_gain_cb,
				.unmute = vcp_vol_ctlr_aics_unmute_cb,
				.mute = vcp_vol_ctlr_aics_mute_cb,
				.set_manual_mode = vcp_vol_ctlr_aics_set_manual_mode_cb,
				.set_auto_mode = vcp_vol_ctlr_aics_set_auto_mode_cb,
			};

			vol_ctlr_insts[i].aics[j] = bt_aics_client_free_instance_get();

			__ASSERT(vol_ctlr_insts[i].aics[j],
				 "Could not allocate AICS client instance");

			bt_aics_client_cb_register(vol_ctlr_insts[i].aics[j], &aics_cb);
		}
	}
#endif /* CONFIG_BT_VCP_VOL_CTLR_AICS */
}

int bt_vcp_vol_ctlr_discover(struct bt_conn *conn, struct bt_vcp_vol_ctlr **out_vol_ctlr)
{
	static bool initialized;
	struct bt_vcp_vol_ctlr *vol_ctlr;
	int err;

	/*
	 * This will initiate a discover procedure. The procedure will do the
	 * following sequence:
	 * 1) Primary discover for the vol_ctlr
	 * 2) Characteristic discover of the vol_ctlr
	 * 3) Discover services included in vol_ctlr (VOCS and AICS)
	 * 4) For each included service found; discovery of the characteristics
	 * 5) When everything above have been discovered, the callback is called
	 */

	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	CHECKIF(out_vol_ctlr == NULL) {
		LOG_DBG("NULL ctlr");
		return -EINVAL;
	}

	vol_ctlr = vol_ctlr_get_by_conn(conn);

	if (vol_ctlr->busy) {
		return -EBUSY;
	}

	if (!initialized) {
		bt_vcp_vol_ctlr_init();
		initialized = true;
	}

	vcp_vol_ctlr_reset(vol_ctlr);

	memcpy(&vol_ctlr->uuid, BT_UUID_VCS, sizeof(vol_ctlr->uuid));

	vol_ctlr->conn = bt_conn_ref(conn);
	vol_ctlr->discover_params.func = primary_discover_func;
	vol_ctlr->discover_params.uuid = &vol_ctlr->uuid.uuid;
	vol_ctlr->discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	vol_ctlr->discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	vol_ctlr->discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	err = bt_gatt_discover(conn, &vol_ctlr->discover_params);
	if (err == 0) {
		*out_vol_ctlr = vol_ctlr;
	}
	return err;
}

int bt_vcp_vol_ctlr_cb_register(struct bt_vcp_vol_ctlr_cb *cb)
{
	struct bt_vcp_vol_ctlr_cb *tmp;

	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&vcp_vol_ctlr_cbs, tmp, _node) {
		if (tmp == cb) {
			LOG_DBG("Already registered");
			return -EALREADY;
		}
	}

	sys_slist_append(&vcp_vol_ctlr_cbs, &cb->_node);

	return 0;
}

int bt_vcp_vol_ctlr_cb_unregister(struct bt_vcp_vol_ctlr_cb *cb)
{
	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&vcp_vol_ctlr_cbs, &cb->_node)) {
		return -EALREADY;
	}

	return 0;
}

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS) || defined(CONFIG_BT_VCP_VOL_CTLR_AICS)
int bt_vcp_vol_ctlr_included_get(struct bt_vcp_vol_ctlr *vol_ctlr,
			       struct bt_vcp_included *included)
{
	CHECKIF(!included || vol_ctlr == NULL) {
		return -EINVAL;
	}

	memset(included, 0, sizeof(*included));

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
	included->vocs_cnt = vol_ctlr->vocs_inst_cnt;
	included->vocs = vol_ctlr->vocs;
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */

#if defined(CONFIG_BT_VCP_VOL_CTLR_AICS)
	included->aics_cnt = vol_ctlr->aics_inst_cnt;
	included->aics = vol_ctlr->aics;
#endif /* CONFIG_BT_VCP_VOL_CTLR_AICS */

	return 0;
}
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS || CONFIG_BT_VCP_VOL_CTLR_AICS*/

struct bt_vcp_vol_ctlr *bt_vcp_vol_ctlr_get_by_conn(const struct bt_conn *conn)
{
	struct bt_vcp_vol_ctlr *vol_ctlr;

	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn pointer");
		return NULL;
	}

	vol_ctlr = vol_ctlr_get_by_conn(conn);
	if (vol_ctlr->conn == NULL) {
		LOG_DBG("conn %p is not associated with volume controller. Do discovery first",
			(void *)conn);
		return NULL;
	}

	return vol_ctlr;
}

int bt_vcp_vol_ctlr_conn_get(const struct bt_vcp_vol_ctlr *vol_ctlr, struct bt_conn **conn)
{
	CHECKIF(vol_ctlr == NULL) {
		LOG_DBG("NULL vol_ctlr pointer");
		return -EINVAL;
	}

	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn pointer");
		return -EINVAL;
	}

	if (vol_ctlr->conn == NULL) {
		LOG_DBG("vol_ctlr pointer not associated with a connection. "
			"Do discovery first");
		return -ENOTCONN;
	}

	*conn = vol_ctlr->conn;
	return 0;
}

int bt_vcp_vol_ctlr_read_state(struct bt_vcp_vol_ctlr *vol_ctlr)
{
	int err;

	CHECKIF(vol_ctlr == NULL) {
		LOG_DBG("NULL ctlr");
		return -EINVAL;
	}

	if (vol_ctlr->conn == NULL) {
		LOG_DBG("NULL ctlr conn");
		return -EINVAL;
	}

	if (vol_ctlr->state_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	} else if (vol_ctlr->busy) {
		return -EBUSY;
	}

	vol_ctlr->read_params.func = vcp_vol_ctlr_read_vol_state_cb;
	vol_ctlr->read_params.handle_count = 1;
	vol_ctlr->read_params.single.handle = vol_ctlr->state_handle;
	vol_ctlr->read_params.single.offset = 0U;

	err = bt_gatt_read(vol_ctlr->conn, &vol_ctlr->read_params);
	if (err == 0) {
		vol_ctlr->busy = true;
	}

	return err;
}

int bt_vcp_vol_ctlr_read_flags(struct bt_vcp_vol_ctlr *vol_ctlr)
{
	int err;

	CHECKIF(vol_ctlr == NULL) {
		LOG_DBG("NULL ctlr");
		return -EINVAL;
	}

	if (vol_ctlr->conn == NULL) {
		LOG_DBG("NULL ctlr conn");
		return -EINVAL;
	}

	if (vol_ctlr->flag_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	} else if (vol_ctlr->busy) {
		return -EBUSY;
	}

	vol_ctlr->read_params.func = vcp_vol_ctlr_read_flag_cb;
	vol_ctlr->read_params.handle_count = 1;
	vol_ctlr->read_params.single.handle = vol_ctlr->flag_handle;
	vol_ctlr->read_params.single.offset = 0U;

	err = bt_gatt_read(vol_ctlr->conn, &vol_ctlr->read_params);
	if (err == 0) {
		vol_ctlr->busy = true;
	}

	return err;
}

int bt_vcp_vol_ctlr_vol_down(struct bt_vcp_vol_ctlr *vol_ctlr)
{
	return vcp_vol_ctlr_common_vcs_cp(vol_ctlr, BT_VCP_OPCODE_REL_VOL_DOWN);
}

int bt_vcp_vol_ctlr_vol_up(struct bt_vcp_vol_ctlr *vol_ctlr)
{
	return vcp_vol_ctlr_common_vcs_cp(vol_ctlr, BT_VCP_OPCODE_REL_VOL_UP);
}

int bt_vcp_vol_ctlr_unmute_vol_down(struct bt_vcp_vol_ctlr *vol_ctlr)
{
	return vcp_vol_ctlr_common_vcs_cp(vol_ctlr, BT_VCP_OPCODE_UNMUTE_REL_VOL_DOWN);
}

int bt_vcp_vol_ctlr_unmute_vol_up(struct bt_vcp_vol_ctlr *vol_ctlr)
{
	return vcp_vol_ctlr_common_vcs_cp(vol_ctlr, BT_VCP_OPCODE_UNMUTE_REL_VOL_UP);
}

int bt_vcp_vol_ctlr_set_vol(struct bt_vcp_vol_ctlr *vol_ctlr, uint8_t volume)
{
	int err;

	CHECKIF(vol_ctlr == NULL) {
		LOG_DBG("NULL ctlr");
		return -EINVAL;
	}

	if (vol_ctlr->conn == NULL) {
		LOG_DBG("NULL ctlr conn");
		return -EINVAL;
	}

	if (vol_ctlr->control_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	} else if (vol_ctlr->busy) {
		return -EBUSY;
	}

	vol_ctlr->cp_val.cp.opcode = BT_VCP_OPCODE_SET_ABS_VOL;
	vol_ctlr->cp_val.cp.counter = vol_ctlr->state.change_counter;
	vol_ctlr->cp_val.volume = volume;

	vol_ctlr->write_params.offset = 0;
	vol_ctlr->write_params.data = &vol_ctlr->cp_val;
	vol_ctlr->write_params.length = sizeof(vol_ctlr->cp_val);
	vol_ctlr->write_params.handle = vol_ctlr->control_handle;
	vol_ctlr->write_params.func = vcp_vol_ctlr_write_vcs_cp_cb;

	err = bt_gatt_write(vol_ctlr->conn, &vol_ctlr->write_params);
	if (err == 0) {
		vol_ctlr->busy = true;
	}

	return err;
}

int bt_vcp_vol_ctlr_unmute(struct bt_vcp_vol_ctlr *vol_ctlr)
{
	return vcp_vol_ctlr_common_vcs_cp(vol_ctlr, BT_VCP_OPCODE_UNMUTE);
}

int bt_vcp_vol_ctlr_mute(struct bt_vcp_vol_ctlr *vol_ctlr)
{
	return vcp_vol_ctlr_common_vcs_cp(vol_ctlr, BT_VCP_OPCODE_MUTE);
}
