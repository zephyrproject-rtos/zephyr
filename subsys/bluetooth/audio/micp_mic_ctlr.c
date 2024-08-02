/*  Bluetooth MICP - Microphone Control Profile - Microphone Controller */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2023 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/audio/micp.h>
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
#include "micp_internal.h"

LOG_MODULE_REGISTER(bt_micp_mic_ctlr, CONFIG_BT_MICP_MIC_CTLR_LOG_LEVEL);

/* Callback functions */
static sys_slist_t micp_mic_ctlr_cbs = SYS_SLIST_STATIC_INIT(&micp_mic_ctlr_cbs);

static struct bt_micp_mic_ctlr mic_ctlrs[CONFIG_BT_MAX_CONN];
static const struct bt_uuid *mics_uuid = BT_UUID_MICS;

static struct bt_micp_mic_ctlr *mic_ctlr_get_by_conn(const struct bt_conn *conn)
{
	return &mic_ctlrs[bt_conn_index(conn)];
}

static void micp_mic_ctlr_mute_changed(struct bt_micp_mic_ctlr *mic_ctlr, int err, uint8_t mute_val)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->mute) {
			listener->mute(mic_ctlr, err, mute_val);
		}
	}
}

static void micp_mic_ctlr_mute_written(struct bt_micp_mic_ctlr *mic_ctlr, int err, uint8_t mute_val)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (mute_val == BT_MICP_MUTE_UNMUTED) {
			if (listener->unmute_written) {
				listener->unmute_written(mic_ctlr, err);
			}
		} else {
			if (listener->mute_written) {
				listener->mute_written(mic_ctlr, err);
			}
		}
	}
}

static void micp_mic_ctlr_discover_complete(struct bt_micp_mic_ctlr *mic_ctlr, int err)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->discover) {
			uint8_t aics_cnt = 0U;

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
			if (err == 0) {
				aics_cnt = mic_ctlr->aics_inst_cnt;
			}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

			listener->discover(mic_ctlr, err, aics_cnt);
		}
	}
}

static uint8_t mute_notify_handler(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				   const void *data, uint16_t length)
{
	uint8_t *mute_val;
	struct bt_micp_mic_ctlr *mic_ctlr;

	if (conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	mic_ctlr = mic_ctlr_get_by_conn(conn);

	if (data != NULL) {
		if (length == sizeof(*mute_val)) {
			mute_val = (uint8_t *)data;
			LOG_DBG("Mute %u", *mute_val);
			micp_mic_ctlr_mute_changed(mic_ctlr, 0, *mute_val);
		}
	} else {
		LOG_DBG("Invalid length %u (expected %zu)", length, sizeof(*mute_val));
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t micp_mic_ctlr_read_mute_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	struct bt_micp_mic_ctlr *mic_ctlr = mic_ctlr_get_by_conn(conn);
	uint8_t cb_err = err;
	uint8_t mute_val = 0;

	mic_ctlr->busy = false;

	if (err > 0) {
		LOG_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		if (length == sizeof(mute_val)) {
			mute_val = ((uint8_t *)data)[0];
			LOG_DBG("Mute %u", mute_val);
		} else {
			LOG_DBG("Invalid length %u (expected %zu)", length, sizeof(mute_val));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	micp_mic_ctlr_mute_changed(mic_ctlr, cb_err, mute_val);

	return BT_GATT_ITER_STOP;
}

static void micp_mic_ctlr_write_mics_mute_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_write_params *params)
{
	struct bt_micp_mic_ctlr *mic_ctlr = mic_ctlr_get_by_conn(conn);
	uint8_t mute_val = mic_ctlr->mute_val_buf[0];

	LOG_DBG("Write %s (0x%02X)", err ? "failed" : "successful", err);

	mic_ctlr->busy = false;

	micp_mic_ctlr_mute_written(mic_ctlr, err, mute_val);
}

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static struct bt_micp_mic_ctlr *lookup_micp_by_aics(const struct bt_aics *aics)
{
	__ASSERT(aics != NULL, "AICS pointer cannot be NULL");

	for (int i = 0; i < ARRAY_SIZE(mic_ctlrs); i++) {
		for (int j = 0; j < ARRAY_SIZE(mic_ctlrs[i].aics); j++) {
			if (mic_ctlrs[i].aics[j] == aics) {
				return &mic_ctlrs[i];
			}
		}
	}

	return NULL;
}

static void micp_mic_ctlr_aics_state_cb(struct bt_aics *inst, int err, int8_t gain, uint8_t mute,
					uint8_t mode)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.state) {
			listener->aics_cb.state(inst, err, gain, mute, mode);
		}
	}
}

static void micp_mic_ctlr_aics_gain_setting_cb(struct bt_aics *inst, int err, uint8_t units,
					       int8_t minimum, int8_t maximum)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.gain_setting) {
			listener->aics_cb.gain_setting(inst, err, units, minimum, maximum);
		}
	}
}

static void micp_mic_ctlr_aics_type_cb(struct bt_aics *inst, int err, uint8_t type)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.type) {
			listener->aics_cb.type(inst, err, type);
		}
	}
}

static void micp_mic_ctlr_aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.status) {
			listener->aics_cb.status(inst, err, active);
		}
	}
}

static void micp_mic_ctlr_aics_description_cb(struct bt_aics *inst, int err, char *description)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.description) {
			listener->aics_cb.description(inst, err, description);
		}
	}
}

static void micp_mic_ctlr_aics_discover_cb(struct bt_aics *inst, int err)
{
	struct bt_micp_mic_ctlr *mic_ctlr = lookup_micp_by_aics(inst);
	struct bt_micp_mic_ctlr_cb *listener, *next;

	if (mic_ctlr == NULL) {
		LOG_ERR("Could not lookup mic_ctlr from aics");
		micp_mic_ctlr_discover_complete(mic_ctlr, BT_GATT_ERR(BT_ATT_ERR_UNLIKELY));

		return;
	}

	if (err == 0) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(mic_ctlr->conn, &mic_ctlr->discover_params);
	}

	if (err != 0) {
		LOG_DBG("Discover failed (err %d)", err);
		micp_mic_ctlr_discover_complete(mic_ctlr, err);
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.discover) {
			listener->aics_cb.discover(inst, err);
		}
	}
}

static void micp_mic_ctlr_aics_set_gain_cb(struct bt_aics *inst, int err)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.set_gain) {
			listener->aics_cb.set_gain(inst, err);
		}
	}
}

static void micp_mic_ctlr_aics_unmute_cb(struct bt_aics *inst, int err)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.unmute) {
			listener->aics_cb.unmute(inst, err);
		}
	}
}

static void micp_mic_ctlr_aics_mute_cb(struct bt_aics *inst, int err)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.mute) {
			listener->aics_cb.mute(inst, err);
		}
	}
}

static void micp_mic_ctlr_aics_set_manual_mode_cb(struct bt_aics *inst, int err)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.set_manual_mode) {
			listener->aics_cb.set_manual_mode(inst, err);
		}
	}
}

static void micp_mic_ctlr_aics_set_auto_mode_cb(struct bt_aics *inst, int err)
{
	struct bt_micp_mic_ctlr_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&micp_mic_ctlr_cbs, listener, next, _node) {
		if (listener->aics_cb.set_auto_mode) {
			listener->aics_cb.set_auto_mode(inst, err);
		}
	}
}

static uint8_t micp_discover_include_func(
	struct bt_conn *conn, const struct bt_gatt_attr *attr,
	struct bt_gatt_discover_params *params)
{
	struct bt_micp_mic_ctlr *mic_ctlr = mic_ctlr_get_by_conn(conn);

	if (attr == NULL) {
		LOG_DBG("Discover include complete for MICS: %u AICS", mic_ctlr->aics_inst_cnt);
		(void)memset(params, 0, sizeof(*params));

		micp_mic_ctlr_discover_complete(mic_ctlr, 0);

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		struct bt_gatt_include *include = (struct bt_gatt_include *)attr->user_data;

		LOG_DBG("Include UUID %s", bt_uuid_str(include->uuid));

		if (bt_uuid_cmp(include->uuid, BT_UUID_AICS) == 0 &&
		    mic_ctlr->aics_inst_cnt < CONFIG_BT_MICP_MIC_CTLR_MAX_AICS_INST) {
			uint8_t inst_idx;
			int err;
			struct bt_aics_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_aics_discover
			 */
			mic_ctlr->discover_params.start_handle = attr->handle + 1;

			inst_idx = mic_ctlr->aics_inst_cnt++;
			err = bt_aics_discover(conn, mic_ctlr->aics[inst_idx],
					       &param);
			if (err != 0) {
				LOG_DBG("AICS Discover failed (err %d)", err);
				micp_mic_ctlr_discover_complete(mic_ctlr, err);
			}

			return BT_GATT_ITER_STOP;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
static uint8_t micp_discover_func(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	struct bt_micp_mic_ctlr *mic_ctlr = mic_ctlr_get_by_conn(conn);

	if (attr == NULL) {
		int err = 0;

		LOG_DBG("Discovery complete");
		(void)memset(params, 0, sizeof(*params));

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
		/* Discover included services */
		mic_ctlr->discover_params.start_handle = mic_ctlr->start_handle;
		mic_ctlr->discover_params.end_handle = mic_ctlr->end_handle;
		mic_ctlr->discover_params.type = BT_GATT_DISCOVER_INCLUDE;
		mic_ctlr->discover_params.func = micp_discover_include_func;

		err = bt_gatt_discover(conn, &mic_ctlr->discover_params);
		if (err != 0) {
			LOG_DBG("Discover AICS failed (err %d)", err);
			micp_mic_ctlr_discover_complete(mic_ctlr, err);
		}
#else
		micp_mic_ctlr_discover_complete(mic_ctlr, err);
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;
		struct bt_gatt_subscribe_params *sub_params = NULL;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_MICS_MUTE) == 0) {
			LOG_DBG("Mute");
			mic_ctlr->mute_handle = chrc->value_handle;
			sub_params = &mic_ctlr->mute_sub_params;
			sub_params->disc_params = &mic_ctlr->mute_sub_disc_params;
		}

		if (sub_params != NULL) {
			int err;

			/* With ccc_handle == 0 it will use auto discovery */
			sub_params->ccc_handle = 0;
			sub_params->end_handle = mic_ctlr->end_handle;
			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = chrc->value_handle;
			sub_params->notify = mute_notify_handler;
			atomic_set_bit(sub_params->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

			err = bt_gatt_subscribe(conn, sub_params);
			if (err == 0 || err == -EALREADY) {
				LOG_DBG("Subscribed to handle 0x%04X", attr->handle);
			} else {
				LOG_DBG("Could not subscribe to handle 0x%04X: %d", attr->handle,
					err);

				micp_mic_ctlr_discover_complete(mic_ctlr, err);

				return BT_GATT_ITER_STOP;
			}
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t primary_discover_func(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	struct bt_micp_mic_ctlr *mic_ctlr = mic_ctlr_get_by_conn(conn);

	if (attr == NULL) {
		LOG_DBG("Could not find a MICS instance on the server");
		micp_mic_ctlr_discover_complete(mic_ctlr, -ENODATA);

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		struct bt_gatt_service_val *prim_service =
			(struct bt_gatt_service_val *)attr->user_data;
		int err;

		LOG_DBG("Primary discover complete");
		mic_ctlr->start_handle = attr->handle + 1;
		mic_ctlr->end_handle = prim_service->end_handle;

		/* Discover characteristics */
		mic_ctlr->discover_params.uuid = NULL;
		mic_ctlr->discover_params.start_handle = mic_ctlr->start_handle;
		mic_ctlr->discover_params.end_handle = mic_ctlr->end_handle;
		mic_ctlr->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		mic_ctlr->discover_params.func = micp_discover_func;

		err = bt_gatt_discover(conn, &mic_ctlr->discover_params);
		if (err != 0) {
			LOG_DBG("Discover failed (err %d)", err);
			micp_mic_ctlr_discover_complete(mic_ctlr, err);
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void micp_mic_ctlr_reset(struct bt_micp_mic_ctlr *mic_ctlr)
{
	mic_ctlr->start_handle = 0;
	mic_ctlr->end_handle = 0;
	mic_ctlr->mute_handle = 0;
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	mic_ctlr->aics_inst_cnt = 0;
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

	if (mic_ctlr->conn != NULL) {
		struct bt_conn *conn = mic_ctlr->conn;

		bt_conn_unref(conn);
		mic_ctlr->conn = NULL;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_micp_mic_ctlr *mic_ctlr = mic_ctlr_get_by_conn(conn);

	if (mic_ctlr->conn == conn) {
		micp_mic_ctlr_reset(mic_ctlr);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

int bt_micp_mic_ctlr_discover(struct bt_conn *conn, struct bt_micp_mic_ctlr **mic_ctlr_out)
{
	struct bt_micp_mic_ctlr *mic_ctlr;
	int err;

	/*
	 * This will initiate a discover procedure. The procedure will do the
	 * following sequence:
	 * 1) Primary discover for the MICS
	 * 2) Characteristic discover of the MICS
	 * 3) Discover services included in MICS (AICS)
	 * 4) For each included service found; discovery of the characteristics
	 * 5) When everything above have been discovered, the callback is called
	 */

	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	mic_ctlr = mic_ctlr_get_by_conn(conn);

	(void)memset(&mic_ctlr->discover_params, 0,
		     sizeof(mic_ctlr->discover_params));
	micp_mic_ctlr_reset(mic_ctlr);

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	static bool initialized;

	if (!initialized) {
		for (size_t i = 0U; i < ARRAY_SIZE(mic_ctlrs); i++) {
			for (size_t j = 0U; j < ARRAY_SIZE(mic_ctlr->aics); j++) {
				static struct bt_aics_cb aics_cb = {
					.state = micp_mic_ctlr_aics_state_cb,
					.gain_setting = micp_mic_ctlr_aics_gain_setting_cb,
					.type = micp_mic_ctlr_aics_type_cb,
					.status = micp_mic_ctlr_aics_status_cb,
					.description = micp_mic_ctlr_aics_description_cb,
					.discover = micp_mic_ctlr_aics_discover_cb,
					.set_gain = micp_mic_ctlr_aics_set_gain_cb,
					.unmute = micp_mic_ctlr_aics_unmute_cb,
					.mute = micp_mic_ctlr_aics_mute_cb,
					.set_manual_mode = micp_mic_ctlr_aics_set_manual_mode_cb,
					.set_auto_mode = micp_mic_ctlr_aics_set_auto_mode_cb,
				};

				mic_ctlrs[i].aics[j] = bt_aics_client_free_instance_get();

				if (mic_ctlrs[i].aics[j] == NULL) {
					return -ENOMEM;
				}

				bt_aics_client_cb_register(mic_ctlrs[i].aics[j], &aics_cb);
			}
		}

		initialized = true;
	}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

	mic_ctlr->conn = bt_conn_ref(conn);
	mic_ctlr->discover_params.func = primary_discover_func;
	mic_ctlr->discover_params.uuid = mics_uuid;
	mic_ctlr->discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	mic_ctlr->discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	mic_ctlr->discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	err = bt_gatt_discover(conn, &mic_ctlr->discover_params);
	if (err == 0) {
		*mic_ctlr_out = mic_ctlr;
	}

	return err;
}

int bt_micp_mic_ctlr_cb_register(struct bt_micp_mic_ctlr_cb *cb)
{
	struct bt_micp_mic_ctlr_cb *tmp;

	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&micp_mic_ctlr_cbs, tmp, _node) {
		if (tmp == cb) {
			LOG_DBG("Already registered");
			return -EALREADY;
		}
	}

	sys_slist_append(&micp_mic_ctlr_cbs, &cb->_node);

	return 0;
}

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
int bt_micp_mic_ctlr_included_get(struct bt_micp_mic_ctlr *mic_ctlr,
				  struct bt_micp_included *included)
{
	CHECKIF(mic_ctlr == NULL) {
		LOG_DBG("NULL mic_ctlr");
		return -EINVAL;
	}

	CHECKIF(included == NULL) {
		return -EINVAL;
	}

	included->aics_cnt = mic_ctlr->aics_inst_cnt;
	included->aics = mic_ctlr->aics;

	return 0;
}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

struct bt_micp_mic_ctlr *bt_micp_mic_ctlr_get_by_conn(const struct bt_conn *conn)
{
	struct bt_micp_mic_ctlr *mic_ctlr;

	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn pointer");
		return NULL;
	}

	mic_ctlr = mic_ctlr_get_by_conn(conn);
	if (mic_ctlr->conn == NULL) {
		LOG_DBG("conn %p is not associated with microphone controller. Do discovery first",
			(void *)conn);
		return NULL;
	}

	return mic_ctlr;
}

int bt_micp_mic_ctlr_conn_get(const struct bt_micp_mic_ctlr *mic_ctlr, struct bt_conn **conn)
{
	CHECKIF(mic_ctlr == NULL) {
		LOG_DBG("NULL mic_ctlr pointer");
		return -EINVAL;
	}

	if (mic_ctlr->conn == NULL) {
		LOG_DBG("mic_ctlr pointer not associated with a connection. "
		       "Do discovery first");
		return -ENOTCONN;
	}

	*conn = mic_ctlr->conn;
	return 0;
}

int bt_micp_mic_ctlr_mute_get(struct bt_micp_mic_ctlr *mic_ctlr)
{
	int err;

	CHECKIF(mic_ctlr == NULL) {
		LOG_DBG("NULL mic_ctlr");
		return -EINVAL;
	}

	if (mic_ctlr->mute_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	} else if (mic_ctlr->busy) {
		return -EBUSY;
	}

	mic_ctlr->read_params.func = micp_mic_ctlr_read_mute_cb;
	mic_ctlr->read_params.handle_count = 1;
	mic_ctlr->read_params.single.handle = mic_ctlr->mute_handle;
	mic_ctlr->read_params.single.offset = 0U;

	err = bt_gatt_read(mic_ctlr->conn, &mic_ctlr->read_params);
	if (err == 0) {
		mic_ctlr->busy = true;
	}

	return err;
}

int bt_micp_mic_ctlr_write_mute(struct bt_micp_mic_ctlr *mic_ctlr, bool mute)
{
	int err;

	CHECKIF(mic_ctlr == NULL) {
		LOG_DBG("NULL mic_ctlr");
		return -EINVAL;
	}

	if (mic_ctlr->mute_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	} else if (mic_ctlr->busy) {
		return -EBUSY;
	}

	mic_ctlr->mute_val_buf[0] = mute;
	mic_ctlr->write_params.offset = 0;
	mic_ctlr->write_params.data = mic_ctlr->mute_val_buf;
	mic_ctlr->write_params.length = sizeof(mute);
	mic_ctlr->write_params.handle = mic_ctlr->mute_handle;
	mic_ctlr->write_params.func = micp_mic_ctlr_write_mics_mute_cb;

	err = bt_gatt_write(mic_ctlr->conn, &mic_ctlr->write_params);
	if (err == 0) {
		mic_ctlr->busy = true;
	}

	return err;
}

int bt_micp_mic_ctlr_mute(struct bt_micp_mic_ctlr *mic_ctlr)
{
	return bt_micp_mic_ctlr_write_mute(mic_ctlr, true);
}

int bt_micp_mic_ctlr_unmute(struct bt_micp_mic_ctlr *mic_ctlr)
{
	return bt_micp_mic_ctlr_write_mute(mic_ctlr, false);
}
