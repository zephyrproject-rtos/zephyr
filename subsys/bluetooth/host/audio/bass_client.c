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
#include <sys/check.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_BASS_CLIENT)
#define LOG_MODULE_NAME bt_bass_client
#include "common/log.h"

#include "bass_internal.h"
#include "../conn_internal.h"
#include "../hci_core.h"

#define FIRST_HANDLE			0x0001
#define LAST_HANDLE			0xFFFF

#define MINIMUM_RECV_STATE_LEN          15

struct bass_client_instance_t {
	bool discovering;
	bool scanning;
	uint8_t pa_sync;
	uint8_t recv_state_cnt;
	/* Source ID cache so that we can notify application about
	 * which source ID was removed
	 */
	uint8_t src_ids[CONFIG_BT_BASS_CLIENT_RECV_STATE_COUNT];

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t cp_handle;
	uint16_t recv_state_handles[CONFIG_BT_BASS_CLIENT_RECV_STATE_COUNT];

	bool busy;
	struct bt_gatt_subscribe_params recv_state_sub_params
		[CONFIG_BT_BASS_CLIENT_RECV_STATE_COUNT];
	struct bt_gatt_discover_params recv_state_disc_params
		[CONFIG_BT_BASS_CLIENT_RECV_STATE_COUNT];
	struct bt_gatt_read_params read_params;
	struct bt_gatt_write_params write_params;
	struct bt_gatt_discover_params disc_params;
};

static struct bt_bass_client_cb_t *bass_cbs;

static struct bass_client_instance_t bass_client;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

NET_BUF_SIMPLE_DEFINE_STATIC(cp_buf, CONFIG_BT_L2CAP_TX_MTU);

static int16_t lookup_index_by_handle(uint16_t handle)
{
	for (int i = 0; i < ARRAY_SIZE(bass_client.recv_state_handles); i++) {
		if (bass_client.recv_state_handles[i] == handle) {
			return i;
		}
	}

	BT_ERR("Unknown source was removed");

	return -1;
}

static int parse_recv_state(const void *data, uint16_t length,
			    struct bt_bass_recv_state *recv_state)
{
	struct net_buf_simple buf;
	bt_addr_t *addr;

	__ASSERT(recv_state, "NULL receive state");

	if (!data || length == 0) {
		BT_DBG("NULL data");
		return -EINVAL;
	}

	if (length < MINIMUM_RECV_STATE_LEN) {
		BT_DBG("Invalid receive state length %u, expected at least %u",
			length, MINIMUM_RECV_STATE_LEN);
		return -EINVAL;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	memset(recv_state, 0, sizeof(*recv_state));

	recv_state->src_id = net_buf_simple_pull_u8(&buf);
	recv_state->addr.type = net_buf_simple_pull_u8(&buf);
	addr = net_buf_simple_pull_mem(&buf, sizeof(*addr));
	bt_addr_copy(&recv_state->addr.a, addr);
	recv_state->adv_sid = net_buf_simple_pull_u8(&buf);
	recv_state->broadcast_id = net_buf_simple_pull_le24(&buf);
	recv_state->pa_sync_state = net_buf_simple_pull_u8(&buf);
	recv_state->encrypt_state = net_buf_simple_pull_u8(&buf);
	if (recv_state->encrypt_state == BASS_BIG_ENC_STATE_BAD_CODE) {
		uint8_t *broadcast_code;

		if (buf.len < sizeof(recv_state->bad_code) + sizeof(recv_state->num_subgroups)) {
			BT_DBG("Invalid receive state length %u, expected at least %zu",
			       buf.size, (buf.len + sizeof(recv_state->bad_code) +
								sizeof(recv_state->num_subgroups)));
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		broadcast_code = net_buf_simple_pull_mem(&buf, BASS_BROADCAST_CODE_SIZE);
		memcpy(recv_state->bad_code, broadcast_code, sizeof(recv_state->bad_code));
	}

	recv_state->num_subgroups = net_buf_simple_pull_u8(&buf);
	for (int i = 0; i < recv_state->num_subgroups; i++) {
		struct bt_bass_subgroup *subgroup = &recv_state->subgroups[i];
		uint8_t *metadata;

		if (buf.len < sizeof(subgroup->bis_sync)) {
			BT_DBG("Invalid receive state length %u, expected at least %zu",
			       buf.len, buf.len + sizeof(subgroup->bis_sync));
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		subgroup->bis_sync = net_buf_simple_pull_le32(&buf);

		if (buf.len < sizeof(subgroup->metadata_len)) {
			BT_DBG("Invalid receive state length %u, expected at least %zu",
			       buf.len, buf.len + sizeof(subgroup->metadata_len));
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}
		subgroup->metadata_len = net_buf_simple_pull_u8(&buf);

		if (buf.len < subgroup->metadata_len) {
			BT_DBG("Invalid receive state length %u, expected at least %u",
			       buf.len, buf.len + subgroup->metadata_len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (subgroup->metadata_len > sizeof(subgroup->metadata)) {
			BT_DBG("Metadata too long: %u/%zu",
			       subgroup->metadata_len, sizeof(subgroup->metadata));
		}

		metadata = net_buf_simple_pull_mem(&buf, subgroup->metadata_len);
		memcpy(subgroup->metadata, metadata, subgroup->metadata_len);
	}

	if (buf.len != 0) {
		BT_DBG("Invalid receive state length %u, but only %u was parsed",
		       length, length - buf.len);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	return 0;
}

/** @brief Handles notifications and indications from the server */
static uint8_t notify_handler(struct bt_conn *conn,
			      struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_bass_recv_state recv_state;
	int err;
	int16_t index;

	if (!data) {
		BT_DBG("[UNSUBSCRIBED] %u", handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	BT_HEXDUMP_DBG(data, length, "Receive state notification:");

	index = lookup_index_by_handle(handle);
	if (index < 0) {
		BT_DBG("Invalid index");
		return BT_GATT_ITER_STOP;
	}

	if (data && length) {
		err = parse_recv_state(data, length, &recv_state);

		if (err) {
			/* TODO: Likely due to the length. Start a read autonomously */
			BT_WARN("Invalid receive state received");
			return BT_GATT_ITER_STOP;
		}

		bass_client.src_ids[index] = recv_state.src_id;

		if (bass_cbs && bass_cbs->recv_state) {
			bass_cbs->recv_state(conn, 0, &recv_state);
		}
	} else if (bass_cbs && bass_cbs->recv_state_removed) {
		bass_cbs->recv_state_removed(conn, 0,
					     bass_client.src_ids[index]);
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
	struct bt_bass_recv_state recv_state;
	int bass_err = err;
	bool active_recv_state = data && length;

	/* TODO: Split discovery and receive state characteristic read */

	memset(params, 0, sizeof(*params));

	if (!bass_err && active_recv_state) {
		int16_t index;

		index = lookup_index_by_handle(handle);
		if (index < 0) {
			bass_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_HANDLE);
		} else {
			bass_err = parse_recv_state(data, length, &recv_state);

			if (bass_err) {
				BT_DBG("Invalid receive state");
			} else {
				bass_client.src_ids[index] = recv_state.src_id;
			}
		}
	}

	if (bass_err) {
		BT_DBG("err: %d", bass_err);
		if (bass_client.discovering) {
			bass_client.discovering = false;
			if (bass_cbs && bass_cbs->discover) {
				bass_cbs->discover(conn, bass_err, 0);
			}
		} else {
			if (bass_cbs && bass_cbs->recv_state) {
				bass_cbs->recv_state(conn, bass_err, NULL);
			}
		}
	} else if (handle == last_handle) {
		if (bass_client.discovering) {
			bass_client.discovering = false;
			if (bass_cbs && bass_cbs->discover) {
				bass_cbs->discover(conn, bass_err, bass_client.recv_state_cnt);
			}
		} else {
			if (bass_cbs && bass_cbs->recv_state) {
				bass_cbs->recv_state(conn, bass_err, &recv_state);
			}
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
				sub_params = &bass_client.recv_state_sub_params[idx];
				sub_params->disc_params = &bass_client.recv_state_disc_params[idx];
			}
		}

		if (sub_params) {
			/* With ccc_handle == 0 it will use auto discovery */
			sub_params->end_handle = bass_client.end_handle;
			sub_params->ccc_handle = 0;
			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = attr->handle + 1;
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
	uint8_t opcode = net_buf_simple_pull_u8(&cp_buf);

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

static int bt_bass_client_common_cp(struct bt_conn *conn, const struct net_buf_simple *buf)
{
	int err;

	if (!conn) {
		return -ENOTCONN;
	} else if (!bass_client.cp_handle) {
		BT_DBG("Handle not set");
		return -EINVAL;
	}

	bass_client.write_params.offset = 0;
	bass_client.write_params.data = buf->data;
	bass_client.write_params.length = buf->len;
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
	struct bt_uuid_16 adv_uuid;
	uint32_t broadcast_id;

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BT_UUID_SIZE_16 + BT_BASS_BROADCAST_ID_SIZE) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO)) {
		return true;
	}

	broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);

	BT_DBG("Found BIS advertiser with address %s",
	       bt_addr_le_str(info->addr));

	if (bass_cbs && bass_cbs->scan) {
		bass_cbs->scan(info, broadcast_id);
	}

	return false;
}

void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	/* We are only interested in non-connectable periodic advertisers */
	if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) || info->interval == 0) {
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

void bt_bass_client_register_cb(struct bt_bass_client_cb_t *cb)
{
	bass_cbs = cb;
}

int bt_bass_client_scan_start(struct bt_conn *conn, bool start_scan)
{
	struct bass_cp_scan_start_t *cp;
	int err;

	if (!conn) {
		return -ENOTCONN;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	} else if (bass_client.busy) {
		return -EBUSY;
	}

	if (start_scan) {
		static bool cb_registered;

		if (!cb_registered) {
			bt_le_scan_cb_register(&scan_cb);
			cb_registered = true;
		}

		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);

		if (err) {
			BT_DBG("Could not start scan (%d)", err);
			return err;
		} else {
			bass_client.scanning = true;
		}
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&cp_buf);
	cp = net_buf_simple_add(&cp_buf, sizeof(*cp));

	cp->opcode = BASS_OP_SCAN_START;

	return bt_bass_client_common_cp(conn, &cp_buf);
}

int bt_bass_client_scan_stop(struct bt_conn *conn)
{
	struct bass_cp_scan_stop_t *cp;
	int err;

	if (!conn) {
		return 0;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	} else if (bass_client.busy) {
		return -EBUSY;
	}

	if (bass_client.scanning) {
		err = bt_le_scan_stop();
		if (err) {
			BT_DBG("Could not stop scan (%d)", err);
			return err;
		}
		bass_client.scanning = false;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&cp_buf);
	cp = net_buf_simple_add(&cp_buf, sizeof(*cp));

	cp->opcode = BASS_OP_SCAN_STOP;

	return bt_bass_client_common_cp(conn, &cp_buf);
}

int bt_bass_client_add_src(struct bt_conn *conn, struct bt_bass_add_src_param *param)
{
	struct bass_cp_add_src_t *cp;

	if (!conn) {
		return 0;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	} else if (bass_client.busy) {
		return -EBUSY;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&cp_buf);
	cp = net_buf_simple_add(&cp_buf, sizeof(*cp));

	cp->opcode = BASS_OP_ADD_SRC;
	cp->adv_sid = param->adv_sid;
	bt_addr_le_copy(&cp->addr, &param->addr);

	sys_put_le24(param->broadcast_id, cp->broadcast_id);

	if (param->pa_sync) {
		if (BT_FEAT_LE_PAST_SEND(conn->le.features) &&
		    BT_FEAT_LE_PAST_RECV(bt_dev.le.features)) {
			/* TODO: Validate that we are synced to the peer address
			 * before saying to use PAST - Requires additional per_adv_sync API
			 */
			cp->pa_sync = BASS_PA_REQ_SYNC_PAST;
		} else {
			cp->pa_sync = BASS_PA_REQ_SYNC;
		}
	} else {
		cp->pa_sync = BASS_PA_REQ_NO_SYNC;
	}

	cp->pa_interval = sys_cpu_to_le16(param->pa_interval);

	cp->num_subgroups = param->num_subgroups;
	for (int i = 0; i < param->num_subgroups; i++) {
		struct bass_cp_subgroup *subgroup;
		const size_t subgroup_size = sizeof(subgroup->bis_sync) +
						sizeof(subgroup->metadata_len) +
						param->subgroups[i].metadata_len;

		if (cp_buf.len + subgroup_size > cp_buf.size) {
			BT_DBG("MTU is too small to send %zu octets", cp_buf.len + subgroup_size);
			return -EINVAL;
		}

		subgroup = net_buf_simple_add(&cp_buf, subgroup_size);

		subgroup->bis_sync = param->subgroups[i].bis_sync;

		CHECKIF(!param->pa_sync && subgroup->bis_sync) {
			BT_DBG("Only syncing to BIS is not allowed");
			return -EINVAL;
		}

		if (param->subgroups[i].metadata_len) {
			memcpy(subgroup->metadata, param->subgroups[i].metadata,
			       param->subgroups[i].metadata_len);
			subgroup->metadata_len = param->subgroups[i].metadata_len;
		} else {
			subgroup->metadata_len = 0;
		}

	}

	return bt_bass_client_common_cp(conn, &cp_buf);
}


int bt_bass_client_mod_src(struct bt_conn *conn, struct bt_bass_mod_src_param *param)
{
	struct bass_cp_mod_src_t *cp;

	if (!conn) {
		return 0;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	} else if (bass_client.busy) {
		return -EBUSY;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&cp_buf);
	cp = net_buf_simple_add(&cp_buf, sizeof(*cp));

	cp->opcode = BASS_OP_MOD_SRC;
	cp->src_id = param->src_id;

	if (param->pa_sync) {
		if (BT_FEAT_LE_PAST_SEND(conn->le.features) &&
		    BT_FEAT_LE_PAST_RECV(bt_dev.le.features)) {
			cp->pa_sync = BASS_PA_REQ_SYNC_PAST;
		} else {
			cp->pa_sync = BASS_PA_REQ_SYNC;
		}
	} else {
		cp->pa_sync = BASS_PA_REQ_NO_SYNC;
	}
	cp->pa_interval = sys_cpu_to_le16(param->pa_interval);

	cp->num_subgroups = param->num_subgroups;
	for (int i = 0; i < param->num_subgroups; i++) {
		struct bass_cp_subgroup *subgroup;
		const size_t subgroup_size = sizeof(subgroup->bis_sync) +
						sizeof(subgroup->metadata_len) +
						param->subgroups[i].metadata_len;

		if (cp_buf.len + subgroup_size > cp_buf.size) {
			BT_DBG("MTU is too small to send %zu octets", cp_buf.len + subgroup_size);
			return -EINVAL;
		}
		subgroup = net_buf_simple_add(&cp_buf, subgroup_size);

		subgroup->bis_sync = param->subgroups[i].bis_sync;


		CHECKIF(!param->pa_sync && subgroup->bis_sync) {
			BT_DBG("Only syncing to BIS is not allowed");
			return -EINVAL;
		}

		if (param->subgroups[i].metadata_len) {
			memcpy(subgroup->metadata, param->subgroups[i].metadata,
			       param->subgroups[i].metadata_len);
			subgroup->metadata_len = param->subgroups[i].metadata_len;
		} else {
			subgroup->metadata_len = 0;
		}
	}

	return bt_bass_client_common_cp(conn, &cp_buf);
}

int bt_bass_client_broadcast_code(struct bt_conn *conn, uint8_t src_id,
				  uint8_t broadcast_code[16])
{
	struct bass_cp_broadcase_code_t *cp;

	if (!conn) {
		return 0;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	} else if (bass_client.busy) {
		return -EBUSY;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&cp_buf);
	cp = net_buf_simple_add(&cp_buf, sizeof(*cp));

	cp->opcode = BASS_OP_BROADCAST_CODE;
	cp->src_id = src_id;

	memcpy(cp->broadcast_code, broadcast_code, BASS_BROADCAST_CODE_SIZE);

	return bt_bass_client_common_cp(conn, &cp_buf);
}

int bt_bass_client_rem_src(struct bt_conn *conn, uint8_t src_id)
{
	struct bass_cp_rem_src_t *cp;

	if (!conn) {
		return 0;
	} else if (!bass_client.cp_handle) {
		return -EINVAL;
	} else if (bass_client.busy) {
		return -EBUSY;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&cp_buf);
	cp = net_buf_simple_add(&cp_buf, sizeof(*cp));

	cp->opcode = BASS_OP_REM_SRC;
	cp->src_id = src_id;

	return bt_bass_client_common_cp(conn, &cp_buf);
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
