/*  Bluetooth MICS client - Microphone Control Profile - Client */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/audio/mics.h>

#include "mics_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MICS_CLIENT)
#define LOG_MODULE_NAME bt_mics_client
#include "common/log.h"

/* Callback functions */
static struct bt_mics_cb *mics_client_cb;

static struct bt_mics mics_insts[CONFIG_BT_MAX_CONN];
static struct bt_uuid *mics_uuid = BT_UUID_MICS;

bool bt_mics_client_valid_aics_inst(struct bt_mics *mics, struct bt_aics *aics)
{
	if (mics == NULL) {
		return false;
	}

	if (aics == NULL) {
		return false;
	}

	if (!mics->client_instance) {
		return false;
	}

	for (int i = 0; i < ARRAY_SIZE(mics->cli.aics); i++) {
		if (mics->cli.aics[i] == aics) {
			return true;
		}
	}

	return false;
}

static uint8_t mute_notify_handler(struct bt_conn *conn,
				   struct bt_gatt_subscribe_params *params,
				   const void *data, uint16_t length)
{
	uint8_t *mute_val;
	struct bt_mics *mics_inst;

	if (conn == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	mics_inst = &mics_insts[bt_conn_index(conn)];

	if (data != NULL) {
		if (length == sizeof(*mute_val)) {
			mute_val = (uint8_t *)data;
			BT_DBG("Mute %u", *mute_val);
			if (mics_client_cb != NULL &&
			    mics_client_cb->mute != NULL) {
				mics_client_cb->mute(mics_inst, 0, *mute_val);
			}
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*mute_val));

		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t mics_client_read_mute_cb(struct bt_conn *conn, uint8_t err,
					struct bt_gatt_read_params *params,
					const void *data, uint16_t length)
{
	uint8_t cb_err = err;
	uint8_t mute_val = 0;
	struct bt_mics *mics_inst = &mics_insts[bt_conn_index(conn)];

	mics_inst->cli.busy = false;

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

	if (mics_client_cb != NULL && mics_client_cb->mute != NULL) {
		mics_client_cb->mute(mics_inst, cb_err, mute_val);
	}

	return BT_GATT_ITER_STOP;
}

static void mics_client_write_mics_mute_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_write_params *params)
{
	struct bt_mics *mics_inst = &mics_insts[bt_conn_index(conn)];
	uint8_t mute_val = mics_inst->cli.mute_val_buf[0];

	BT_DBG("Write %s (0x%02X)", err ? "failed" : "successful", err);

	mics_inst->cli.busy = false;

	if (mute_val == BT_MICS_MUTE_UNMUTED) {
		if (mics_client_cb != NULL &&
		    mics_client_cb->unmute_write != NULL) {
			mics_client_cb->unmute_write(mics_inst, err);
		}

	} else {
		if (mics_client_cb != NULL &&
		    mics_client_cb->mute_write != NULL) {
			mics_client_cb->mute_write(mics_inst, err);
		}
	}
}

#if defined(CONFIG_BT_MICS_CLIENT_AICS)
static struct bt_mics *lookup_mics_by_aics(const struct bt_aics *aics)
{
	__ASSERT(aics != NULL, "AICS pointer cannot be NULL");

	for (int i = 0; i < ARRAY_SIZE(mics_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(mics_insts[i].cli.aics); j++) {
			if (mics_insts[i].cli.aics[j] == aics) {
				return &mics_insts[i];
			}
		}
	}

	return NULL;
}

static void aics_discover_cb(struct bt_aics *inst, int err)
{
	struct bt_mics *mics_inst = lookup_mics_by_aics(inst);

	if (err == 0) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(mics_inst->cli.conn,
				       &mics_inst->cli.discover_params);
	}

	if (err != 0) {
		BT_DBG("Discover failed (err %d)", err);
		if (mics_client_cb != NULL &&
		    mics_client_cb->discover != NULL) {
			mics_client_cb->discover(mics_inst, err, 0);
		}
	}
}
#endif /* CONFIG_BT_MICS_CLIENT_AICS */

static uint8_t mics_discover_include_func(
	struct bt_conn *conn, const struct bt_gatt_attr *attr,
	struct bt_gatt_discover_params *params)
{
	struct bt_mics *mics_inst = &mics_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		BT_DBG("Discover include complete for MICS: %u AICS",
		       mics_inst->cli.aics_inst_cnt);
		(void)memset(params, 0, sizeof(*params));

		if (mics_client_cb != NULL &&
		    mics_client_cb->discover != NULL) {
			mics_client_cb->discover(mics_inst, 0, 0);
		}

		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		struct bt_gatt_include *include = (struct bt_gatt_include *)attr->user_data;

		BT_DBG("Include UUID %s", bt_uuid_str(include->uuid));

		if (bt_uuid_cmp(include->uuid, BT_UUID_AICS) == 0 &&
		    mics_inst->cli.aics_inst_cnt < CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
			uint8_t inst_idx;
			int err;
			struct bt_aics_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_aics_discover
			 */
			mics_inst->cli.discover_params.start_handle = attr->handle + 1;

			inst_idx = mics_inst->cli.aics_inst_cnt++;
			err = bt_aics_discover(conn, mics_inst->cli.aics[inst_idx],
					       &param);
			if (err != 0) {
				BT_DBG("AICS Discover failed (err %d)", err);
				if (mics_client_cb != NULL &&
				    mics_client_cb->discover != NULL) {
					mics_client_cb->discover(mics_inst, err,
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
static uint8_t mics_discover_func(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	struct bt_mics *mics_inst = &mics_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		int err = 0;

		BT_DBG("Setup complete for MICS");
		(void)memset(params, 0, sizeof(*params));
		if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
			/* Discover included services */
			mics_inst->cli.discover_params.start_handle = mics_inst->cli.start_handle;
			mics_inst->cli.discover_params.end_handle = mics_inst->cli.end_handle;
			mics_inst->cli.discover_params.type = BT_GATT_DISCOVER_INCLUDE;
			mics_inst->cli.discover_params.func = mics_discover_include_func;

			err = bt_gatt_discover(conn,
					       &mics_inst->cli.discover_params);
			if (err != 0) {
				BT_DBG("Discover failed (err %d)", err);
				if (mics_client_cb != NULL &&
				    mics_client_cb->discover != NULL) {
					mics_client_cb->discover(mics_inst, err, 0);
				}
			}
		} else {
			if (mics_client_cb != NULL &&
			    mics_client_cb->discover != NULL) {
				mics_client_cb->discover(mics_inst, err, 0);
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
			mics_inst->cli.mute_handle = chrc->value_handle;
			sub_params = &mics_inst->cli.mute_sub_params;
			sub_params->disc_params = &mics_inst->cli.mute_sub_disc_params;
		}

		if (sub_params != NULL) {
			int err;

			/* With ccc_handle == 0 it will use auto discovery */
			sub_params->ccc_handle = 0;
			sub_params->end_handle = mics_inst->cli.end_handle;
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
	struct bt_mics *mics_inst = &mics_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		BT_DBG("Could not find a MICS instance on the server");
		if (mics_client_cb != NULL &&
		    mics_client_cb->discover != NULL) {
			mics_client_cb->discover(mics_inst, -ENODATA, 0);
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		struct bt_gatt_service_val *prim_service =
			(struct bt_gatt_service_val *)attr->user_data;
		int err;

		BT_DBG("Primary discover complete");
		mics_inst->cli.start_handle = attr->handle + 1;
		mics_inst->cli.end_handle = prim_service->end_handle;

		/* Discover characteristics */
		mics_inst->cli.discover_params.uuid = NULL;
		mics_inst->cli.discover_params.start_handle = mics_inst->cli.start_handle;
		mics_inst->cli.discover_params.end_handle = mics_inst->cli.end_handle;
		mics_inst->cli.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		mics_inst->cli.discover_params.func = mics_discover_func;

		err = bt_gatt_discover(conn, &mics_inst->cli.discover_params);
		if (err != 0) {
			BT_DBG("Discover failed (err %d)", err);
			if (mics_client_cb != NULL &&
			    mics_client_cb->discover != NULL) {
				mics_client_cb->discover(mics_inst, err, 0);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void mics_client_reset(struct bt_mics *mics)
{
	mics->cli.start_handle = 0;
	mics->cli.end_handle = 0;
	mics->cli.mute_handle = 0;
	mics->cli.aics_inst_cnt = 0;

	if (mics->cli.conn != NULL) {
		struct bt_conn *conn = mics->cli.conn;

		/* It's okay if this fails. In case of disconnect, we can't
		 * unsubscribe and it will just fail.
		 * In case that we reset due to another call of the discover
		 * function, we will unsubscribe (regardless of bonding state)
		 * to accommodate the new discovery values.
		 */
		(void)bt_gatt_unsubscribe(conn, &mics->cli.mute_sub_params);

		bt_conn_unref(conn);
		mics->cli.conn = NULL;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_mics *mics = &mics_insts[bt_conn_index(conn)];

	if (mics->cli.conn == conn) {
		mics_client_reset(mics);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

int bt_mics_discover(struct bt_conn *conn, struct bt_mics **mics)
{
	static bool initialized;
	struct bt_mics *mics_inst;
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

	mics_inst = &mics_insts[bt_conn_index(conn)];

	(void)memset(&mics_inst->cli.discover_params, 0,
		     sizeof(mics_inst->cli.discover_params));
	mics_client_reset(mics_inst);

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) &&
	    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		for (int i = 0; i < ARRAY_SIZE(mics_inst->cli.aics); i++) {
			if (!initialized) {
				mics_inst->cli.aics[i] = bt_aics_client_free_instance_get();

				if (mics_inst->cli.aics[i] == NULL) {
					return -ENOMEM;
				}

				bt_aics_client_cb_register(mics_inst->cli.aics[i],
							   &mics_client_cb->aics_cb);
			}
		}
	}

	mics_inst->cli.conn = bt_conn_ref(conn);
	mics_inst->client_instance = true;
	mics_inst->cli.discover_params.func = primary_discover_func;
	mics_inst->cli.discover_params.uuid = mics_uuid;
	mics_inst->cli.discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	mics_inst->cli.discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	mics_inst->cli.discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	initialized = true;

	err = bt_gatt_discover(conn, &mics_inst->cli.discover_params);
	if (err == 0) {
		*mics = mics_inst;
	}

	return err;
}

int bt_mics_client_cb_register(struct bt_mics_cb *cb)
{
#if defined(CONFIG_BT_MICS_CLIENT_AICS)
	struct bt_aics_cb *aics_cb = NULL;

	if (cb != NULL) {
		CHECKIF(cb->aics_cb.discover != NULL) {
			BT_ERR("AICS discover callback shall not be set");
			return -EINVAL;
		}
		cb->aics_cb.discover = aics_discover_cb;

		aics_cb = &cb->aics_cb;
	}

	for (int i = 0; i < ARRAY_SIZE(mics_insts); i++) {
		for (int j = 0; j < ARRAY_SIZE(mics_insts[i].cli.aics); j++) {
			struct bt_aics *aics = mics_insts[i].cli.aics[j];

			if (aics != NULL) {
				bt_aics_client_cb_register(aics, aics_cb);
			}
		}
	}
#endif /* CONFIG_BT_MICS_CLIENT_AICS */

	mics_client_cb = cb;

	return 0;
}

int bt_mics_client_included_get(struct bt_mics *mics,
				struct bt_mics_included *included)
{
	CHECKIF(mics == NULL) {
		BT_DBG("NULL mics");
		return -EINVAL;
	}

	CHECKIF(included == NULL) {
		return -EINVAL;
	}

	included->aics_cnt = mics->cli.aics_inst_cnt;
	included->aics = mics->cli.aics;

	return 0;
}

int bt_mics_client_conn_get(const struct bt_mics *mics, struct bt_conn **conn)
{
	CHECKIF(mics == NULL) {
		BT_DBG("NULL mics pointer");
		return -EINVAL;
	}

	if (!mics->client_instance) {
		BT_DBG("mics pointer shall be client instance");
		return -EINVAL;
	}

	if (mics->cli.conn == NULL) {
		BT_DBG("mics pointer not associated with a connection. "
		       "Do discovery first");
		return -ENOTCONN;
	}

	*conn = mics->cli.conn;
	return 0;
}

int bt_mics_client_mute_get(struct bt_mics *mics)
{
	int err;

	CHECKIF(mics == NULL) {
		BT_DBG("NULL mics");
		return -EINVAL;
	}

	if (mics->cli.mute_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mics->cli.busy) {
		return -EBUSY;
	}

	mics->cli.read_params.func = mics_client_read_mute_cb;
	mics->cli.read_params.handle_count = 1;
	mics->cli.read_params.single.handle = mics->cli.mute_handle;
	mics->cli.read_params.single.offset = 0U;

	err = bt_gatt_read(mics->cli.conn, &mics->cli.read_params);
	if (err == 0) {
		mics->cli.busy = true;
	}

	return err;
}

int bt_mics_client_write_mute(struct bt_mics *mics, bool mute)
{
	int err;

	CHECKIF(mics == NULL) {
		BT_DBG("NULL mics");
		return -EINVAL;
	}

	if (mics->cli.mute_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mics->cli.busy) {
		return -EBUSY;
	}

	mics->cli.mute_val_buf[0] = mute;
	mics->cli.write_params.offset = 0;
	mics->cli.write_params.data = mics->cli.mute_val_buf;
	mics->cli.write_params.length = sizeof(mute);
	mics->cli.write_params.handle = mics->cli.mute_handle;
	mics->cli.write_params.func = mics_client_write_mics_mute_cb;

	err = bt_gatt_write(mics->cli.conn, &mics->cli.write_params);
	if (err == 0) {
		mics->cli.busy = true;
	}

	return err;
}

int bt_mics_client_mute(struct bt_mics *mics)
{
	return bt_mics_client_write_mute(mics, true);
}

int bt_mics_client_unmute(struct bt_mics *mics)
{
	return bt_mics_client_write_mute(mics, false);
}
