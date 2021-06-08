/*  Bluetooth MICS client - Microphone Control Profile - Client */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <sys/check.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio/mics.h>

#include "mics_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MICS_CLIENT)
#define LOG_MODULE_NAME bt_mics_client
#include "common/log.h"

/* Callback functions */
static struct bt_mics_cb *mics_client_cb;

static struct bt_mics_client mics_insts[CONFIG_BT_MAX_CONN];
static struct bt_uuid *mics_uuid = BT_UUID_MICS;

bool bt_mics_client_valid_aics_inst(struct bt_conn *conn, struct bt_aics *aics)
{
	uint8_t conn_index;

	CHECKIF(conn == NULL) {
		return -EINVAL;
	}

	if (aics == NULL) {
		return false;
	}

	conn_index = bt_conn_index(conn);

	for (int i = 0; i < ARRAY_SIZE(mics_insts[conn_index].aics); i++) {
		if (mics_insts[conn_index].aics[i] == aics) {
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

	if (data != NULL) {
		if (length == sizeof(*mute_val)) {
			mute_val = (uint8_t *)data;
			BT_DBG("Mute %u", *mute_val);
			if (mics_client_cb != NULL &&
			    mics_client_cb->mute != NULL) {
				mics_client_cb->mute(conn, 0, *mute_val);
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
	uint8_t *mute_val = NULL;
	struct bt_mics_client *mics_inst = &mics_insts[bt_conn_index(conn)];

	mics_inst->busy = false;

	if (err > 0) {
		BT_DBG("err: 0x%02X", err);
	} else if (data != NULL) {
		if (length == sizeof(*mute_val)) {
			mute_val = (uint8_t *)data;
			BT_DBG("Mute %u", *mute_val);
		} else {
			BT_DBG("Invalid length %u (expected %zu)",
			       length, sizeof(*mute_val));
			cb_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}
	}

	if (mics_client_cb != NULL && mics_client_cb->mute != NULL) {
		mics_client_cb->mute(conn, cb_err, *mute_val);
	}

	return BT_GATT_ITER_STOP;
}

static void mics_client_write_mics_mute_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_write_params *params)
{
	struct bt_mics_client *mics_inst = &mics_insts[bt_conn_index(conn)];
	uint8_t mute_val = mics_inst->mute_val_buf[0];

	BT_DBG("Write %s (0x%02X)", err ? "failed" : "successful", err);

	mics_inst->busy = false;

	if (mute_val == BT_MICS_MUTE_UNMUTED) {
		if (mics_client_cb != NULL &&
		    mics_client_cb->unmute_write != NULL) {
			mics_client_cb->unmute_write(conn, err);
		}

	} else {
		if (mics_client_cb != NULL &&
		    mics_client_cb->mute_write != NULL) {
			mics_client_cb->mute_write(conn, err);
		}
	}
}

static void aics_discover_cb(struct bt_conn *conn, struct bt_aics *inst,
			     int err)
{
	struct bt_mics_client *mics_inst = &mics_insts[bt_conn_index(conn)];

	if (err == 0) {
		/* Continue discovery of included services */
		err = bt_gatt_discover(conn, &mics_inst->discover_params);
	}

	if (err != 0) {
		BT_DBG("Discover failed (err %d)", err);
		if (mics_client_cb != NULL &&
		    mics_client_cb->discover != NULL) {
			mics_client_cb->discover(conn, err, 0);
		}
	}
}

static uint8_t mics_discover_include_func(
	struct bt_conn *conn, const struct bt_gatt_attr *attr,
	struct bt_gatt_discover_params *params)
{
	struct bt_mics_client *mics_inst = &mics_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		BT_DBG("Discover include complete for MICS: %u AICS",
		       mics_inst->aics_inst_cnt);
		(void)memset(params, 0, sizeof(*params));

		if (mics_client_cb != NULL &&
		    mics_client_cb->discover != NULL) {
			mics_client_cb->discover(conn, 0, 0);
		}

		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_INCLUDE) {
		struct bt_gatt_include *include = (struct bt_gatt_include *)attr->user_data;

		BT_DBG("Include UUID %s", bt_uuid_str(include->uuid));

		if (bt_uuid_cmp(include->uuid, BT_UUID_AICS) == 0 &&
		    mics_inst->aics_inst_cnt < CONFIG_BT_MICS_CLIENT_MAX_AICS_INST) {
			uint8_t inst_idx;
			int err;
			struct bt_aics_discover_param param = {
				.start_handle = include->start_handle,
				.end_handle = include->end_handle,
			};

			/* Update discover params so we can continue where we
			 * left off after bt_aics_discover
			 */
			mics_inst->discover_params.start_handle = attr->handle + 1;

			inst_idx = mics_inst->aics_inst_cnt++;
			err = bt_aics_discover(conn, mics_inst->aics[inst_idx],
					       &param);
			if (err != 0) {
				BT_DBG("AICS Discover failed (err %d)", err);
				if (mics_client_cb != NULL &&
				    mics_client_cb->discover != NULL) {
					mics_client_cb->discover(conn, err, 0);
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
	struct bt_mics_client *mics_inst = &mics_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		int err = 0;

		BT_DBG("Setup complete for MICS");
		(void)memset(params, 0, sizeof(*params));
		if (CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
			/* Discover included services */
			mics_inst->discover_params.start_handle = mics_inst->start_handle;
			mics_inst->discover_params.end_handle = mics_inst->end_handle;
			mics_inst->discover_params.type = BT_GATT_DISCOVER_INCLUDE;
			mics_inst->discover_params.func = mics_discover_include_func;

			err = bt_gatt_discover(conn,
					       &mics_inst->discover_params);
			if (err != 0) {
				BT_DBG("Discover failed (err %d)", err);
				if (mics_client_cb != NULL &&
				    mics_client_cb->discover != NULL) {
					mics_client_cb->discover(conn, err, 0);
				}
			}
		} else {
			if (mics_client_cb != NULL &&
			    mics_client_cb->discover != NULL) {
				mics_client_cb->discover(conn, err, 0);
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
			mics_inst->mute_handle = chrc->value_handle;
			sub_params = &mics_inst->mute_sub_params;
			sub_params->disc_params = &mics_inst->mute_sub_disc_params;
		}

		if (sub_params != NULL) {
			int err;

			/* With ccc_handle == 0 it will use auto discovery */
			sub_params->ccc_handle = 0;
			sub_params->end_handle = mics_inst->end_handle;
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
	struct bt_mics_client *mics_inst = &mics_insts[bt_conn_index(conn)];

	if (attr == NULL) {
		BT_DBG("Could not find a MICS instance on the server");
		if (mics_client_cb != NULL &&
		    mics_client_cb->discover != NULL) {
			mics_client_cb->discover(conn, -ENODATA, 0);
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		struct bt_gatt_service_val *prim_service =
			(struct bt_gatt_service_val *)attr->user_data;
		int err;

		BT_DBG("Primary discover complete");
		mics_inst->start_handle = attr->handle + 1;
		mics_inst->end_handle = prim_service->end_handle;

		/* Discover characteristics */
		mics_inst->discover_params.uuid = NULL;
		mics_inst->discover_params.start_handle = mics_inst->start_handle;
		mics_inst->discover_params.end_handle = mics_inst->end_handle;
		mics_inst->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		mics_inst->discover_params.func = mics_discover_func;

		err = bt_gatt_discover(conn, &mics_inst->discover_params);
		if (err != 0) {
			BT_DBG("Discover failed (err %d)", err);
			if (mics_client_cb != NULL &&
			    mics_client_cb->discover != NULL) {
				mics_client_cb->discover(conn, err, 0);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void mics_client_reset(struct bt_conn *conn)
{
	struct bt_mics_client *mics_inst = &mics_insts[bt_conn_index(conn)];

	mics_inst->start_handle = 0;
	mics_inst->end_handle = 0;
	mics_inst->mute_handle = 0;
	mics_inst->aics_inst_cnt = 0;

	/* It's okay if this fails */
	(void)bt_gatt_unsubscribe(conn, &mics_inst->mute_sub_params);
}

int bt_mics_discover(struct bt_conn *conn)
{
	static bool initialized;
	struct bt_mics_client *mics_inst;
	/*
	 * This will initiate a discover procedure. The procedure will do the
	 * following sequence:
	 * 1) Primary discover for the MICS
	 * 2) Characteristic discover of the MICS
	 * 3) Discover services included in MICS (AICS)
	 * 4) For each included service found; discovery of the characteristiscs
	 * 5) When everything above have been discovered, the callback is called
	 */

	CHECKIF(conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	mics_inst = &mics_insts[bt_conn_index(conn)];

	(void)memset(&mics_inst->discover_params, 0,
		     sizeof(mics_inst->discover_params));
	mics_client_reset(conn);

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) &&
	    CONFIG_BT_MICS_CLIENT_MAX_AICS_INST > 0) {
		for (int i = 0; i < ARRAY_SIZE(mics_inst->aics); i++) {
			if (!initialized) {
				mics_inst->aics[i] = bt_aics_client_free_instance_get();

				if (mics_inst->aics[i] == NULL) {
					return -ENOMEM;
				}

				bt_aics_client_cb_register(mics_inst->aics[i],
							   &mics_client_cb->aics_cb);
			}
		}
	}

	mics_inst->discover_params.func = primary_discover_func;
	mics_inst->discover_params.uuid = mics_uuid;
	mics_inst->discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	mics_inst->discover_params.start_handle = BT_ATT_FIRST_ATTTRIBUTE_HANDLE;
	mics_inst->discover_params.end_handle = BT_ATT_LAST_ATTTRIBUTE_HANDLE;

	initialized = true;

	return bt_gatt_discover(conn, &mics_inst->discover_params);
}

int bt_mics_client_cb_register(struct bt_mics_cb *cb)
{
	int i, j;

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT)) {
		struct bt_aics_cb *aics_cb = NULL;

		if (cb != NULL) {
			CHECKIF(cb->aics_cb.discover != NULL) {
				BT_ERR("AICS discover callback shall not be set");
				return -EINVAL;
			}
			cb->aics_cb.discover = aics_discover_cb;

			aics_cb = &cb->aics_cb;
		}

		for (i = 0; i < ARRAY_SIZE(mics_insts); i++) {
			for (j = 0; j < ARRAY_SIZE(mics_insts[i].aics); j++) {
				if (mics_insts[i].aics[j] != NULL) {
					bt_aics_client_cb_register(mics_insts[i].aics[j],
								   aics_cb);
				}
			}
		}
	}

	mics_client_cb = cb;

	return 0;
}

int bt_mics_client_included_get(struct bt_conn *conn,
					struct bt_mics_included *included)
{
	struct bt_mics_client *mics_inst;

	CHECKIF(conn == NULL) {
		return -EINVAL;
	}

	CHECKIF(included == NULL) {
		return -EINVAL;
	}

	mics_inst = &mics_insts[bt_conn_index(conn)];

	included->aics_cnt = mics_inst->aics_inst_cnt;
	included->aics = mics_inst->aics;

	return 0;
}

int bt_mics_client_mute_get(struct bt_conn *conn)
{
	int err;
	struct bt_mics_client *mics_inst;

	CHECKIF(conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	mics_inst = &mics_insts[bt_conn_index(conn)];

	if (mics_inst->mute_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mics_inst->busy) {
		return -EBUSY;
	}

	mics_inst->read_params.func = mics_client_read_mute_cb;
	mics_inst->read_params.handle_count = 1;
	mics_inst->read_params.single.handle = mics_inst->mute_handle;
	mics_inst->read_params.single.offset = 0U;

	err = bt_gatt_read(conn, &mics_inst->read_params);
	if (err == 0) {
		mics_inst->busy = true;
	}

	return err;
}

int bt_mics_client_write_mute(struct bt_conn *conn, bool mute)
{
	int err;
	struct bt_mics_client *mics_inst;

	CHECKIF(conn == NULL) {
		BT_DBG("NULL conn");
		return -EINVAL;
	}

	mics_inst = &mics_insts[bt_conn_index(conn)];

	if (mics_inst->mute_handle == 0) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (mics_inst->busy) {
		return -EBUSY;
	}

	mics_inst->mute_val_buf[0] = mute;
	mics_inst->write_params.offset = 0;
	mics_inst->write_params.data = mics_inst->mute_val_buf;
	mics_inst->write_params.length = sizeof(mute);
	mics_inst->write_params.handle = mics_inst->mute_handle;
	mics_inst->write_params.func = mics_client_write_mics_mute_cb;

	err = bt_gatt_write(conn, &mics_inst->write_params);
	if (err == 0) {
		mics_inst->busy = true;
	}

	return err;
}

int bt_mics_client_mute(struct bt_conn *conn)
{
	return bt_mics_client_write_mute(conn, true);
}

int bt_mics_client_unmute(struct bt_conn *conn)
{
	return bt_mics_client_write_mute(conn, false);
}
