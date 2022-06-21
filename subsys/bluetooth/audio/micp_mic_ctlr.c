/*  Bluetooth MICP - Microphone Input Control Profile - Microphone Controller */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/types.h>

#include <zephyr/sys/check.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/micp.h>

#include "micp_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MICP_MIC_CTLR)
#define LOG_MODULE_NAME bt_micp_mic_ctlr
#include "common/log.h"

/* Callback functions */
static struct bt_micp_mic_ctlr_cb *micp_mic_ctlr_cb;

static struct bt_micp micp_insts[CONFIG_BT_MAX_CONN];
static struct bt_uuid *mics_uuid = BT_UUID_MICS;

static uint8_t mute_notify_handler(struct bt_conn *conn,
				   struct bt_gatt_subscribe_params *params,
				   const void *data, uint16_t length)
{
	uint8_t *mute_val;
	struct bt_micp *micp_inst;

	if (conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	micp_inst = &micp_insts[bt_conn_index(conn)];

	if (data != NULL) {
		if (length == sizeof(*mute_val)) {
			mute_val = (uint8_t *)data;
			BT_DBG("Mute %u", *mute_val);
			if (micp_mic_ctlr_cb != NULL &&
			    micp_mic_ctlr_cb->mute != NULL) {
				micp_mic_ctlr_cb->mute(micp_inst, 0, *mute_val);
			}
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*mute_val));

		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t micp_mic_ctlr_read_mute_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint8_t mute_val = 0;
	struct bt_micp *micp_inst = &micp_insts[bt_conn_index(conn)];

	micp_inst->cli.busy = false;

	if (err > 0) {
		BT_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		if (length == sizeof(mute_val)) {
			mute_val = ((uint8_t *)data)[0];
			BT_DBG("Mute %u", mute_val);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(mute_val));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (micp_mic_ctlr_cb != NULL && micp_mic_ctlr_cb->mute != NULL) {
		micp_mic_ctlr_cb->mute(micp_inst, cb_err, mute_val);
	}

	return BT_GATT_ITER_STOP;
}

static void micp_mic_ctlr_write_mics_mute_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_write_params *params)
{
	struct bt_micp *micp_inst = &micp_insts[bt_conn_index(conn)];
	uint8_t mute_val = micp_inst->cli.mute_val_buf[0];

	BT_DBG("Write %s (0x%02X)", err ? "failed" : "successful", err);

	micp_inst->cli.busy = false;

	if (mute_val == BT_MICP_MUTE_UNMUTED) {
		if (micp_mic_ctlr_cb != NULL &&
		    micp_mic_ctlr_cb->unmute_written != NULL) {
			micp_mic_ctlr_cb->unmute_written(micp_inst, err);
		}

	} else {
		if (micp_mic_ctlr_cb != NULL &&
		    micp_mic_ctlr_cb->mute_written != NULL) {
			micp_mic_ctlr_cb->mute_written(micp_inst, err);
		}
	}
}

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static struct bt_micp *lookup_micp_by_aics(const struct bt_aics *aics)
{
	__ASSERT(aics != NULL, "AICS pointer cannot be NULL");

	for (int i = 0; i < ARRAY_SIZE(micp_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(micp_insts[i].cli.aics); j++) {
			if (micp_insts[i].cli.aics[j] == aics) {
				return &micp_insts[i];
			}
		}
	}

	return NULL;
}

static void aics_discover_cb(struct bt_aics *inst, int err)
{
	struct bt_micp *micp_inst = lookup_micp_by_aics(inst);

	if (err == 0) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(micp_inst->cli.conn,
				       &micp_inst->cli.discover_params);
	}

	if (err != 0) {
		BT_DBG("Discover failed (err %d)", err);
		if (micp_mic_ctlr_cb != NULL &&
		    micp_mic_ctlr_cb->discover != NULL) {
			micp_mic_ctlr_cb->discover(micp_inst, err, 0);
		}
	}
}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

static uint8_t micp_discover_include_func(
	struct bt_conn *conn, const struct bt_gatt_attr *attr,
	struct bt_gatt_discover_params *params)
{
	struct bt_micp *micp_inst = &micp_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		BT_DBG("Discover include complete for MICP: %u AICS",
		       micp_inst->cli.aics_inst_cnt);
		(void)memset(params, 0, sizeof(*params));

		if (micp_mic_ctlr_cb != NULL &&
		    micp_mic_ctlr_cb->discover != NULL) {
			micp_mic_ctlr_cb->discover(micp_inst, 0,
						   micp_inst->cli.aics_inst_cnt);
		}

		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		struct bt_gatt_include *include = (struct bt_gatt_include *)attr->user_data;

		BT_DBG("Include UUID %s", bt_uuid_str(include->uuid));

		if (bt_uuid_cmp(include->uuid, BT_UUID_AICS) == 0 &&
		    micp_inst->cli.aics_inst_cnt < CONFIG_BT_MICP_MIC_CTLR_MAX_AICS_INST) {
			uint8_t inst_idx;
			int err;
			struct bt_aics_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_aics_discover
			 */
			micp_inst->cli.discover_params.start_handle = attr->handle + 1;

			inst_idx = micp_inst->cli.aics_inst_cnt++;
			err = bt_aics_discover(conn, micp_inst->cli.aics[inst_idx],
					       &param);
			if (err != 0) {
				BT_DBG("AICS Discover failed (err %d)", err);
				if (micp_mic_ctlr_cb != NULL &&
				    micp_mic_ctlr_cb->discover != NULL) {
					micp_mic_ctlr_cb->discover(micp_inst, err,
								 0);
				}
			}
			return BT_GATT_ITER_STOP;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
static uint8_t micp_discover_func(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	struct bt_micp *micp_inst = &micp_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		int err = 0;

		BT_DBG("Setup complete for MICP");
		(void)memset(params, 0, sizeof(*params));
		if (CONFIG_BT_MICP_MIC_CTLR_MAX_AICS_INST > 0) {
			/* Discover included services */
			micp_inst->cli.discover_params.start_handle = micp_inst->cli.start_handle;
			micp_inst->cli.discover_params.end_handle = micp_inst->cli.end_handle;
			micp_inst->cli.discover_params.type = BT_GATT_DISCOVER_INCLUDE;
			micp_inst->cli.discover_params.func = micp_discover_include_func;

			err = bt_gatt_discover(conn,
					       &micp_inst->cli.discover_params);
			if (err != 0) {
				BT_DBG("Discover failed (err %d)", err);
				if (micp_mic_ctlr_cb != NULL &&
				    micp_mic_ctlr_cb->discover != NULL) {
					micp_mic_ctlr_cb->discover(micp_inst, err, 0);
				}
			}
		} else {
			if (micp_mic_ctlr_cb != NULL &&
			    micp_mic_ctlr_cb->discover != NULL) {
				micp_mic_ctlr_cb->discover(micp_inst, err, 0);
			}
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;
		struct bt_gatt_subscribe_params *sub_params = NULL;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_MICS_MUTE) == 0) {
			BT_DBG("Mute");
			micp_inst->cli.mute_handle = chrc->value_handle;
			sub_params = &micp_inst->cli.mute_sub_params;
			sub_params->disc_params = &micp_inst->cli.mute_sub_disc_params;
		}

		if (sub_params != NULL) {
			int err;

			/* With ccc_handle == 0 it will use auto discovery */
			sub_params->ccc_handle = 0;
			sub_params->end_handle = micp_inst->cli.end_handle;
			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = chrc->value_handle;
			sub_params->notify = mute_notify_handler;

			err = bt_gatt_subscribe(conn, sub_params);
			if (err == 0) {
				BT_DBG("Subscribed to handle 0x%04X",
				       attr->handle);
			} else {
				BT_DBG("Could not subscribe to handle 0x%04X: %d",
				       attr->handle, err);
			}
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t primary_discover_func(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	struct bt_micp *micp_inst = &micp_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		BT_DBG("Could not find a MICS instance on the server");
		if (micp_mic_ctlr_cb != NULL &&
		    micp_mic_ctlr_cb->discover != NULL) {
			micp_mic_ctlr_cb->discover(micp_inst, -ENODATA, 0);
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		struct bt_gatt_service_val *prim_service =
			(struct bt_gatt_service_val *)attr->user_data;
		int err;

		BT_DBG("Primary discover complete");
		micp_inst->cli.start_handle = attr->handle + 1;
		micp_inst->cli.end_handle = prim_service->end_handle;

		/* Discover characteristics */
		micp_inst->cli.discover_params.uuid = NULL;
		micp_inst->cli.discover_params.start_handle = micp_inst->cli.start_handle;
		micp_inst->cli.discover_params.end_handle = micp_inst->cli.end_handle;
		micp_inst->cli.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		micp_inst->cli.discover_params.func = micp_discover_func;

		err = bt_gatt_discover(conn, &micp_inst->cli.discover_params);
		if (err != 0) {
			BT_DBG("Discover failed (err %d)", err);
			if (micp_mic_ctlr_cb != NULL &&
			    micp_mic_ctlr_cb->discover != NULL) {
				micp_mic_ctlr_cb->discover(micp_inst, err, 0);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void micp_mic_ctlr_reset(struct bt_micp *micp)
{
	micp->cli.start_handle = 0;
	micp->cli.end_handle = 0;
	micp->cli.mute_handle = 0;
	micp->cli.aics_inst_cnt = 0;

	if (micp->cli.conn != NULL) {
		struct bt_conn *conn = micp->cli.conn;

		/* It's okay if this fails. In case of disconnect, we can't
		 * unsubscribe and it will just fail.
		 * In case that we reset due to another call of the discover
		 * function, we will unsubscribe (regardless of bonding state)
		 * to accommodate the new discovery values.
		 */
		(void)bt_gatt_unsubscribe(conn, &micp->cli.mute_sub_params);

		bt_conn_unref(conn);
		micp->cli.conn = NULL;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_micp *micp = &micp_insts[bt_conn_index(conn)];

	if (micp->cli.conn == conn) {
		micp_mic_ctlr_reset(micp);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

int bt_micp_mic_ctlr_discover(struct bt_conn *conn, struct bt_micp **micp)
{
	struct bt_micp *micp_inst;
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
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	micp_inst = &micp_insts[bt_conn_index(conn)];

	(void)memset(&micp_inst->cli.discover_params, 0,
		     sizeof(micp_inst->cli.discover_params));
	micp_mic_ctlr_reset(micp_inst);

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	static bool initialized;

	if (!initialized) {
		for (int i = 0; i < ARRAY_SIZE(micp_inst->cli.aics); i++) {
			micp_inst->cli.aics[i] = bt_aics_client_free_instance_get();

			if (micp_inst->cli.aics[i] == NULL) {
				return -ENOMEM;
			}

			bt_aics_client_cb_register(micp_inst->cli.aics[i],
						   &micp_mic_ctlr_cb->aics_cb);
		}
	}

	initialized = true;
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

	micp_inst->cli.conn = bt_conn_ref(conn);
	micp_inst->client_instance = true;
	micp_inst->cli.discover_params.func = primary_discover_func;
	micp_inst->cli.discover_params.uuid = mics_uuid;
	micp_inst->cli.discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	micp_inst->cli.discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	micp_inst->cli.discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	err = bt_gatt_discover(conn, &micp_inst->cli.discover_params);
	if (err == 0) {
		*micp = micp_inst;
	}

	return err;
}

int bt_micp_mic_ctlr_cb_register(struct bt_micp_mic_ctlr_cb *cb)
{
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	struct bt_aics_cb *aics_cb = NULL;

	if (cb != NULL) {
		CHECKIF(cb->aics_cb.discover != NULL) {
			BT_ERR("AICS discover callback shall not be set");
			return -EINVAL;
		}
		cb->aics_cb.discover = aics_discover_cb;

		aics_cb = &cb->aics_cb;
	}

	for (int i = 0; i < ARRAY_SIZE(micp_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(micp_insts[i].cli.aics); j++) {
			struct bt_aics *aics = micp_insts[i].cli.aics[j];

			if (aics != NULL) {
				bt_aics_client_cb_register(aics, aics_cb);
			}
		}
	}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

	micp_mic_ctlr_cb = cb;

	return 0;
}

int bt_micp_mic_ctlr_included_get(struct bt_micp *micp,
				  struct bt_micp_included *included)
{
	CHECKIF(micp == NULL) {
		BT_DBG("NULL micp");
		return -EINVAL;
	}

	CHECKIF(included == NULL) {
		return -EINVAL;
	}

	included->aics_cnt = micp->cli.aics_inst_cnt;
	included->aics = micp->cli.aics;

	return 0;
}

int bt_micp_mic_ctlr_conn_get(const struct bt_micp *micp, struct bt_conn **conn)
{
	CHECKIF(micp == NULL) {
		BT_DBG("NULL micp pointer");
		return -EINVAL;
	}

	if (!micp->client_instance) {
		BT_DBG("micp pointer shall be client instance");
		return -EINVAL;
	}

	if (micp->cli.conn == NULL) {
		BT_DBG("micp pointer not associated with a connection. "
		       "Do discovery first");
		return -ENOTCONN;
	}

	*conn = micp->cli.conn;
	return 0;
}

int bt_micp_mic_ctlr_mute_get(struct bt_micp *micp)
{
	int err;

	CHECKIF(micp == NULL) {
		BT_DBG("NULL micp");
		return -EINVAL;
	}

	if (micp->cli.mute_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (micp->cli.busy) {
		return -EBUSY;
	}

	micp->cli.read_params.func = micp_mic_ctlr_read_mute_cb;
	micp->cli.read_params.handle_count = 1;
	micp->cli.read_params.single.handle = micp->cli.mute_handle;
	micp->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(micp->cli.conn, &micp->cli.read_params);
	if (err == 0) {
		micp->cli.busy = true;
	}

	return err;
}

int bt_micp_mic_ctlr_write_mute(struct bt_micp *micp, bool mute)
{
	int err;

	CHECKIF(micp == NULL) {
		BT_DBG("NULL micp");
		return -EINVAL;
	}

	if (micp->cli.mute_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (micp->cli.busy) {
		return -EBUSY;
	}

	micp->cli.mute_val_buf[0] = mute;
	micp->cli.write_params.offset = 0;
	micp->cli.write_params.data = micp->cli.mute_val_buf;
	micp->cli.write_params.length = sizeof(mute);
	micp->cli.write_params.handle = micp->cli.mute_handle;
	micp->cli.write_params.func = micp_mic_ctlr_write_mics_mute_cb;

	err = bt_gatt_write(micp->cli.conn, &micp->cli.write_params);
	if (err == 0) {
		micp->cli.busy = true;
	}

	return err;
}

int bt_micp_mic_ctlr_mute(struct bt_micp *micp)
{
	return bt_micp_mic_ctlr_write_mute(micp, true);
}

int bt_micp_mic_ctlr_unmute(struct bt_micp *micp)
{
	return bt_micp_mic_ctlr_write_mute(micp, false);
}
