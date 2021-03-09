/*  Bluetooth BASS - Call Control Profile - Client */

/*
 * Copyright (c) 2019 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/buf.h>
#include <sys/byteorder.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_BASS_CLIENT)
#define LOG_MODULE_NAME bt_bass_client
#include "common/log.h"

#include "bass_internal.h"
#include "../conn_internal.h"
#include "../hci_core.h"

#define FIRST_HANDLE			0x0001
#define LAST_HANDLE			0xFFFF

struct bass_client_instance_t {
	bool discovering;
	uint8_t pa_sync;
	uint8_t recv_state_cnt;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t cp_handle;
	uint16_t recv_state_handles[CONFIG_BT_BASS_CLIENT_RECV_STATE_COUNT];

	bool busy;
	struct bt_gatt_subscribe_params recv_state_sub_params
		[CONFIG_BT_BASS_CLIENT_RECV_STATE_COUNT];
	struct bt_gatt_read_params read_params;
	struct bt_gatt_write_params write_params;
	struct bt_gatt_discover_params disc_params;
	uint8_t write_buf[sizeof(union bass_cp_t)];
};

static struct bt_bass_client_cb_t *bass_cbs;

static struct bass_client_instance_t bass_client;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static int parse_recv_state(const void *data, uint16_t length,
			    struct bass_recv_state_t *recv_state)
{
	struct bass_recv_state_t *new_recv_state =
		(struct bass_recv_state_t *)data;
	int err = 0;

	if (!recv_state || !data || length == 0) {
		return -EINVAL;
	}

	/* Length shall be between the min and max size, and match the
	 * metadata length field
	 */
	if (length <= sizeof(*recv_state) &&
	    length >= (sizeof(*recv_state) - sizeof(recv_state->metadata)) &&
	    length == BASS_ACTUAL_SIZE(*new_recv_state)) {
		memcpy(recv_state, data, length);

		BT_DBG("src_id %u, PA %u, BIS %u, encrypt %u, "
		       "addr %s, sid %u, metadata_len %u",
		       recv_state->src_id, recv_state->pa_sync_state,
		       recv_state->bis_sync_state, recv_state->big_enc,
		       bt_addr_le_str(&recv_state->addr),
		       recv_state->adv_sid, recv_state->metadata_len);

	} else {
		BT_DBG("Invalid receive state length %u, expected %u",
		       length, BASS_ACTUAL_SIZE(*new_recv_state));
		err = BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	return err;
}

/** @brief Handles notifications and indications from the server */
static uint8_t notify_handler(struct bt_conn *conn,
			      struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bass_recv_state_t recv_state;
	int err;

	if (!data) {
		BT_DBG("[UNSUBSCRIBED] %u", handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	BT_HEXDUMP_DBG(data, length, "Receive state notification:");

	err = parse_recv_state(data, length, &recv_state);

	if (err) {
		BT_DBG("Invalid receive state");
		return BT_GATT_ITER_STOP;
	}

	if (!BASS_RECV_STATE_EMPTY(&recv_state)) {
		if (bass_cbs && bass_cbs->recv_state) {
			bass_cbs->recv_state(conn, 0, &recv_state);
		}
	} else if (bass_cbs && bass_cbs->recv_state_removed) {
		bass_cbs->recv_state_removed(conn, 0, &recv_state);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t read_recv_state_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	uint16_t handle = params->single.handle;
	uint8_t last_handle_index = bass_client.recv_state_cnt - 1;
	uint16_t last_handle = bass_client.recv_state_handles[last_handle_index];
	struct bass_recv_state_t recv_state;

	memset(params, 0, sizeof(*params));

	if (!err) {
		err = parse_recv_state(data, length, &recv_state);

		if (err) {
			BT_DBG("Invalid receive state");
		}
	}

	if (err) {
		BT_DBG("err: 0x%02X", err);
		if (bass_client.discovering) {
			bass_client.discovering = false;
			if (bass_cbs && bass_cbs->discover) {
				bass_cbs->discover(conn, err, 0);
			}
		} else {
			if (bass_cbs && bass_cbs->recv_state) {
				bass_cbs->recv_state(conn, err, NULL);
			}
		}
	} else if (handle == last_handle) {
		if (bass_client.discovering) {
			bass_client.discovering = false;
			if (bass_cbs && bass_cbs->discover) {
				bass_cbs->discover(conn, 0,
						   bass_client.recv_state_cnt);
			}
		}

		if (bass_cbs && bass_cbs->recv_state) {
			bass_cbs->recv_state(conn, err, &recv_state);
		}
	} else {
		for (int i = 0; i < bass_client.recv_state_cnt; i++) {
			if (handle == bass_client.recv_state_handles[i]) {
				bt_bass_client_read_recv_state(conn, i + 1);
				break;
			}
		}
	}

	return BT_GATT_ITER_STOP;
}

/**
 * @brief This will discover all characteristics on the server, retrieving the
 * handles of the writeable characteristics and subscribing to all notify and
 * indicate characteristics.
 */
static uint8_t char_discover_func(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *sub_params = NULL;
	int err;

	if (!attr) {
		BT_DBG("Found %u BASS receive states",
		       bass_client.recv_state_cnt);
		(void)memset(params, 0, sizeof(*params));
		err = bt_bass_client_read_recv_state(conn, 0);

		if (err) {
			bass_client.discovering = false;
			if (bass_cbs && bass_cbs->discover) {
				bass_cbs->discover(conn, err, 0);
			}
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		struct bt_gatt_chrc *chrc =
			(struct bt_gatt_chrc *)attr->user_data;

		if (!bt_uuid_cmp(chrc->uuid, BT_UUID_BASS_CONTROL_POINT)) {
			BT_DBG("Control Point");
			bass_client.cp_handle = attr->handle + 1;
		} else if (!bt_uuid_cmp(chrc->uuid, BT_UUID_BASS_RECV_STATE)) {
			if (bass_client.recv_state_cnt <
				CONFIG_BT_BASS_CLIENT_RECV_STATE_COUNT) {
				uint8_t idx = bass_client.recv_state_cnt++;

				BT_DBG("Receive State %u",
				       bass_client.recv_state_cnt);
				bass_client.recv_state_handles[idx] =
					attr->handle + 1;
				sub_params =
					&bass_client.recv_state_sub_params[idx];
			}
		}

		if (sub_params) {
			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = attr->handle + 1;
			sub_params->ccc_handle = attr->handle + 2;
			sub_params->notify = notify_handler;
			err = bt_gatt_subscribe(conn, sub_params);

			if (err) {
				BT_DBG("Could not subscribe to handle 0x%04x",
				       sub_params->value_handle);

				bass_client.discovering = false;
				if (bass_cbs && bass_cbs->discover) {
					bass_cbs->discover(conn, err, 0);
				}
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
static uint8_t primary_char_discover_func(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	int err;
	struct bt_gatt_service_val *prim_service;

	if (!attr) {
		BT_DBG("Could not discover BASS");
		(void)memset(params, 0, sizeof(*params));

		bass_client.discovering = false;

		if (bass_cbs && bass_cbs->discover) {
			err = BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
			bass_cbs->discover(conn, err, 0);
		}
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		bass_client.start_handle = attr->handle + 1;
		bass_client.end_handle = prim_service->end_handle;

		bass_client.disc_params.uuid = NULL;
		bass_client.disc_params.start_handle = bass_client.start_handle;
		bass_client.disc_params.end_handle = bass_client.end_handle;
		bass_client.disc_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		bass_client.disc_params.func = char_discover_func;

		err = bt_gatt_discover(conn, &bass_client.disc_params);
		if (err) {
			BT_DBG("Discover failed (err %d)", err);
			bass_client.discovering = false;

			if (bass_cbs && bass_cbs->discover) {
				bass_cbs->discover(conn, err, 0);
			}
		}
	}

	return BT_GATT_ITER_STOP;
}

static void bass_client_write_cp_cb(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_write_params *params)
{
	uint8_t opcode = bass_client.write_buf[0];

	bass_client.busy = false;

	if (!bass_cbs) {
		return;
	}

	switch (opcode) {
	case BASS_OP_SCAN_STOP:
		if (bass_cbs->scan_stop) {
			bass_cbs->scan_stop(conn, err);
		}
		break;
	case BASS_OP_SCAN_START:
		if (bass_cbs->scan_start) {
			bass_cbs->scan_start(conn, err);
		}
		break;
	case BASS_OP_ADD_SRC:
		if (bass_cbs->add_src) {
			bass_cbs->add_src(conn, err);
		}
		break;
	case BASS_OP_MOD_SRC:
		if (bass_cbs->mod_src) {
			bass_cbs->mod_src(conn, err);
		}
		break;
	case BASS_OP_BROADCAST_CODE:
		if (bass_cbs->broadcast_code) {
			bass_cbs->broadcast_code(conn, err);
		}
		break;
	case BASS_OP_REM_SRC:
		if (bass_cbs->rem_src) {
			bass_cbs->rem_src(conn, err);
		}
		break;
	default:
		BT_DBG("Unknown opcode 0x%02x", opcode);
		break;
	}
}

static int bt_bass_client_common_cp(struct bt_conn *conn, union bass_cp_t *cp,
				    uint8_t data_len)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	} else if (!bass_client.cp_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	} else if (bass_client.busy) {
		return -EBUSY;
	}

	memcpy(bass_client.write_buf, cp, data_len);
	bass_client.write_params.offset = 0;
	bass_client.write_params.data = bass_client.write_buf;
	bass_client.write_params.length = data_len;
	bass_client.write_params.handle = bass_client.cp_handle;
	bass_client.write_params.func = bass_client_write_cp_cb;

	err = bt_gatt_write(conn, &bass_client.write_params);
	if (!err) {
		bass_client.busy = true;
	}
	return err;
}


static bool broadcaster_found(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;

	switch (data->type) {
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		if (data->data_len % sizeof(uint16_t) != 0U) {
			BT_DBG("AD malformed\n");
			return true;
		}

		for (int i = 0; i < data->data_len; i += sizeof(uint16_t)) {
			struct bt_uuid *adv_uuid;
			uint16_t u16;

			memcpy(&u16, &data->data[i], sizeof(u16));
			adv_uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
			if (bt_uuid_cmp(adv_uuid, BT_UUID_BROADCAST_AUDIO)) {
				continue;
			}

			BT_DBG("Found BIS advertiser with address %s",
			       bt_addr_le_str(info->addr));

			if (bass_cbs && bass_cbs->scan) {
				bass_cbs->scan(info);
			}

			return false;
		}
	}

	return true;
}

void scan_recv(const struct bt_le_scan_recv_info *info,
	       struct net_buf_simple *ad)
{

	/* We're only interested in connectable events */
	if (info->adv_type != BT_GAP_ADV_TYPE_EXT_ADV ||
	    info->adv_props != BT_GAP_ADV_PROP_EXT_ADV) {
		return;
	} else if (info->interval == 0) {
		/* No periodic advertisements */
		return;
	}

	bt_data_parse(ad, broadcaster_found, (void *)info);
}

static struct bt_le_scan_cb scan_cb = {
	.recv = scan_recv
};

/****************************** PUBLIC API ******************************/

int bt_bass_client_discover(struct bt_conn *conn)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	/* Discover BASS on peer, setup handles and notify */
	memset(&bass_client, 0, sizeof(bass_client));
	memcpy(&uuid, BT_UUID_BASS, sizeof(uuid));
	bass_client.disc_params.func = primary_char_discover_func;
	bass_client.disc_params.uuid = &uuid.uuid;
	bass_client.disc_params.type = BT_GATT_DISCOVER_PRIMARY;
	bass_client.disc_params.start_handle = FIRST_HANDLE;
	bass_client.disc_params.end_handle = LAST_HANDLE;
	err = bt_gatt_discover(conn, &bass_client.disc_params);
	if (err) {
		return err;
	}
	bass_client.discovering = true;
	return 0;
}

int bt_bass_client_scan_start(struct bt_conn *conn)
{
	union bass_cp_t cp = { .opcode = BASS_OP_SCAN_START };
	int err;

	if (!conn) {
		return -ENOTCONN;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	}

	bt_le_scan_cb_register(&scan_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);

	if (err) {
		BT_WARN("Could not start scan (%d)", err);

		/* If it's because we're already scanning, let the application
		 * deal with the scan results
		 */
		if (err != EALREADY) {
			return err;
		}
	}

	return bt_bass_client_common_cp(conn, &cp, sizeof(cp.scan_start));
}

int bt_bass_client_scan_stop(struct bt_conn *conn)
{
	union bass_cp_t cp = { .opcode = BASS_OP_SCAN_STOP };
	int err;

	/* TODO: Stop scan for BIS */
	if (!conn) {
		return 0;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	}

	err = bt_le_scan_stop();
	if (err) {
		BT_DBG("Could not stop scan (%d)", err);

		if (err != EALREADY) {
			return err;
		}
	}

	return bt_bass_client_common_cp(conn, &cp, sizeof(cp.scan_stop));
}

void bt_bass_client_register_cb(struct bt_bass_client_cb_t *cb)
{
	bass_cbs = cb;
}

int bt_bass_client_add_src(struct bt_conn *conn, const bt_addr_le_t *addr,
			   uint8_t adv_sid, bool sync_pa, uint32_t sync_bis,
			   uint8_t metadata_len, uint8_t *metadata)
{
	union bass_cp_t cp;

	if (!conn) {
		return 0;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	} else if (!sync_pa && sync_bis) {
		BT_DBG("Only syncing to BIS is not allowed");
		return -EINVAL;
	}

	cp.add_src.opcode = BASS_OP_ADD_SRC;
	cp.add_src.adv_sid = adv_sid;
	cp.add_src.bis_sync = sync_bis;
	bt_addr_le_copy(&cp.add_src.addr, addr);

	if (sync_pa) {
		if (BT_FEAT_LE_PAST_SEND(conn->le.features) &&
		    BT_FEAT_LE_PAST_RECV(bt_dev.le.features)) {
			/* TODO: Validate that we are synced to the peer address
			 * before saying to use PAST - Requires additional per_adv_sync API
			 */
			cp.add_src.pa_sync = BASS_PA_REQ_SYNC_PAST;
		} else {
			cp.add_src.pa_sync = BASS_PA_REQ_SYNC;
		}
	} else {
		cp.add_src.pa_sync = BASS_PA_REQ_NO_SYNC;
	}

	if (metadata_len && metadata) {
		memcpy(cp.add_src.metadata, metadata, metadata_len);
		cp.add_src.metadata_len = metadata_len;
	} else {
		cp.add_src.metadata_len = 0;
	}

	return bt_bass_client_common_cp(conn, &cp,
					BASS_ACTUAL_SIZE(cp.add_src));
}


int bt_bass_client_mod_src(struct bt_conn *conn, uint8_t src_id,
			   bool sync_pa, uint32_t sync_bis,
			   uint8_t metadata_len, uint8_t *metadata)
{
	union bass_cp_t cp;

	if (!conn) {
		return 0;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	} else if (!sync_pa && sync_bis) {
		BT_DBG("Only syncing to BIS is not allowed");
		return -EINVAL;
	}

	cp.mod_src.opcode = BASS_OP_MOD_SRC;
	cp.mod_src.src_id = src_id;
	cp.mod_src.bis_sync = sync_bis;

	if (sync_pa) {
		if (BT_FEAT_LE_PAST_SEND(conn->le.features) &&
		    BT_FEAT_LE_PAST_RECV(bt_dev.le.features)) {
			cp.mod_src.pa_sync = BASS_PA_REQ_SYNC_PAST;
		} else {
			cp.mod_src.pa_sync = BASS_PA_REQ_SYNC;
		}
	} else {
		cp.mod_src.pa_sync = BASS_PA_REQ_NO_SYNC;
	}

	if (metadata_len && metadata) {
		memcpy(cp.mod_src.metadata, metadata, metadata_len);
		cp.mod_src.metadata_len = metadata_len;
	} else {
		cp.mod_src.metadata_len = 0;
	}

	return bt_bass_client_common_cp(conn, &cp,
					BASS_ACTUAL_SIZE(cp.mod_src));
}

int bt_bass_client_broadcast_code(struct bt_conn *conn, uint8_t src_id,
				  uint8_t broadcast_code[16])
{
	union bass_cp_t cp;

	if (!conn) {
		return 0;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	}

	cp.broadcast_code.opcode = BASS_OP_BROADCAST_CODE;
	cp.broadcast_code.src_id = src_id;
	memcpy(cp.broadcast_code.broadcast_code, broadcast_code,
	       BASS_BROADCAST_CODE_SIZE);

	return bt_bass_client_common_cp(conn, &cp, sizeof(cp.broadcast_code));
}

int bt_bass_client_rem_src(struct bt_conn *conn, uint8_t src_id)
{
	union bass_cp_t cp;

	if (!conn) {
		return 0;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	}

	cp.rem_src.opcode = BASS_OP_REM_SRC;
	cp.rem_src.src_id = src_id;

	return bt_bass_client_common_cp(conn, &cp, sizeof(cp.rem_src));
}

int bt_bass_client_read_recv_state(struct bt_conn *conn, uint8_t idx)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	}

	if (!bass_client.recv_state_handles[idx]) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	bass_client.read_params.func = read_recv_state_cb;
	bass_client.read_params.handle_count = 1;
	bass_client.read_params.single.handle =
		bass_client.recv_state_handles[idx];

	err = bt_gatt_read(conn, &bass_client.read_params);
	if (err) {
		memset(&bass_client.read_params, 0,
		       sizeof(bass_client.read_params));
	}
	return err;
}
