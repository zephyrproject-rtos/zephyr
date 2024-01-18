/*  Bluetooth BAP Broadcast Assistant */

/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bap_broadcast_assistant, CONFIG_BT_BAP_BROADCAST_ASSISTANT_LOG_LEVEL);

#include "common/bt_str.h"

#include "bap_internal.h"
#include "../host/conn_internal.h"
#include "../host/hci_core.h"

#define MINIMUM_RECV_STATE_LEN          15

struct bap_broadcast_assistant_instance {
	bool discovering;
	bool scanning;
	uint8_t pa_sync;
	uint8_t recv_state_cnt;
	/* Source ID cache so that we can notify application about
	 * which source ID was removed
	 */
	uint8_t src_ids[CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT];
	/** Cached PAST available */
	bool past_avail[CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT];

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t cp_handle;
	uint16_t recv_state_handles[CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT];

	bool busy;
	struct bt_gatt_subscribe_params recv_state_sub_params
		[CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT];
	struct bt_gatt_discover_params recv_state_disc_params
		[CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT];
	struct bt_gatt_read_params read_params;
	struct bt_gatt_write_params write_params;
	struct bt_gatt_discover_params disc_params;

	struct k_work_delayable bap_read_work;
	struct bt_conn *conn;
	uint16_t long_read_handle;
};

static struct bt_bap_broadcast_assistant_cb *broadcast_assistant_cbs;

static struct bap_broadcast_assistant_instance broadcast_assistant;
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

#define ATT_BUF_SIZE BT_ATT_MAX_ATTRIBUTE_LEN
NET_BUF_SIMPLE_DEFINE_STATIC(att_buf, ATT_BUF_SIZE);

static int16_t lookup_index_by_handle(uint16_t handle)
{
	for (int i = 0; i < ARRAY_SIZE(broadcast_assistant.recv_state_handles); i++) {
		if (broadcast_assistant.recv_state_handles[i] == handle) {
			return i;
		}
	}

	LOG_ERR("Unknown handle 0x%04x", handle);

	return -1;
}

static bool past_available(const struct bt_conn *conn,
			   const bt_addr_le_t *adv_addr,
			   uint8_t sid)
{
	return BT_FEAT_LE_PAST_RECV(conn->le.features) &&
	       BT_FEAT_LE_PAST_SEND(bt_dev.le.features) &&
	       bt_le_per_adv_sync_lookup_addr(adv_addr, sid) != NULL;
}

static int parse_recv_state(const void *data, uint16_t length,
			    struct bt_bap_scan_delegator_recv_state *recv_state)
{
	struct net_buf_simple buf;
	bt_addr_t *addr;

	__ASSERT(recv_state, "NULL receive state");

	if (data == NULL || length == 0) {
		LOG_DBG("NULL data");
		return -EINVAL;
	}

	if (length < MINIMUM_RECV_STATE_LEN) {
		LOG_DBG("Invalid receive state length %u, expected at least %u",
			length, MINIMUM_RECV_STATE_LEN);
		return -EINVAL;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	(void)memset(recv_state, 0, sizeof(*recv_state));

	recv_state->src_id = net_buf_simple_pull_u8(&buf);
	recv_state->addr.type = net_buf_simple_pull_u8(&buf);
	addr = net_buf_simple_pull_mem(&buf, sizeof(*addr));
	bt_addr_copy(&recv_state->addr.a, addr);
	recv_state->adv_sid = net_buf_simple_pull_u8(&buf);
	recv_state->broadcast_id = net_buf_simple_pull_le24(&buf);
	recv_state->pa_sync_state = net_buf_simple_pull_u8(&buf);
	recv_state->encrypt_state = net_buf_simple_pull_u8(&buf);
	if (recv_state->encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE) {
		uint8_t *broadcast_code;
		const size_t minimum_size = sizeof(recv_state->bad_code) +
					    sizeof(recv_state->num_subgroups);

		if (buf.len < minimum_size) {
			LOG_DBG("Invalid receive state length %u, expected at least %zu",
			       buf.len, minimum_size);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		broadcast_code = net_buf_simple_pull_mem(&buf,
							 BT_AUDIO_BROADCAST_CODE_SIZE);
		(void)memcpy(recv_state->bad_code, broadcast_code,
			     sizeof(recv_state->bad_code));
	}

	recv_state->num_subgroups = net_buf_simple_pull_u8(&buf);
	for (int i = 0; i < recv_state->num_subgroups; i++) {
		struct bt_bap_scan_delegator_subgroup *subgroup = &recv_state->subgroups[i];
		uint8_t *metadata;

		if (buf.len < sizeof(subgroup->bis_sync)) {
			LOG_DBG("Invalid receive state length %u, expected at least %zu",
			       buf.len, buf.len + sizeof(subgroup->bis_sync));
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		subgroup->bis_sync = net_buf_simple_pull_le32(&buf);

		if (buf.len < sizeof(subgroup->metadata_len)) {
			LOG_DBG("Invalid receive state length %u, expected at least %zu",
			       buf.len, buf.len + sizeof(subgroup->metadata_len));
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}
		subgroup->metadata_len = net_buf_simple_pull_u8(&buf);

		if (buf.len < subgroup->metadata_len) {
			LOG_DBG("Invalid receive state length %u, expected at least %u",
			       buf.len, buf.len + subgroup->metadata_len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (subgroup->metadata_len > sizeof(subgroup->metadata)) {
			LOG_DBG("Metadata too long: %u/%zu",
			       subgroup->metadata_len,
			       sizeof(subgroup->metadata));
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		metadata = net_buf_simple_pull_mem(&buf,
						   subgroup->metadata_len);
		(void)memcpy(subgroup->metadata, metadata,
			     subgroup->metadata_len);
	}

	if (buf.len != 0) {
		LOG_DBG("Invalid receive state length %u, but only %u was parsed",
		       length, length - buf.len);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	return 0;
}

static void bap_long_op_reset(void)
{
	broadcast_assistant.busy = false;
	broadcast_assistant.long_read_handle = 0;
	net_buf_simple_reset(&att_buf);
}

static uint8_t parse_and_send_recv_state(struct bt_conn *conn, uint16_t handle,
					 const void *data, uint16_t length,
					 struct bt_bap_scan_delegator_recv_state *recv_state)
{
	int err;
	int16_t index;

	err = parse_recv_state(data, length, recv_state);
	if (err != 0) {
		LOG_WRN("Invalid receive state received");

		return BT_GATT_ITER_STOP;
	}

	index = lookup_index_by_handle(handle);
	if (index < 0) {
		LOG_DBG("Invalid index");

		return BT_GATT_ITER_STOP;
	}

	broadcast_assistant.src_ids[index] = recv_state->src_id;
	broadcast_assistant.past_avail[index] = past_available(conn,
							       &recv_state->addr,
							       recv_state->adv_sid);

	if (broadcast_assistant_cbs != NULL &&
	    broadcast_assistant_cbs->recv_state != NULL) {
		broadcast_assistant_cbs->recv_state(conn, 0, recv_state);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t broadcast_assistant_bap_ntf_read_func(struct bt_conn *conn, uint8_t err,
						     struct bt_gatt_read_params *read,
						     const void *data, uint16_t length)
{
	struct bt_bap_scan_delegator_recv_state recv_state;
	uint16_t handle = read->single.handle;
	uint16_t data_length;

	LOG_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err) {
		LOG_DBG("Failed to read: %u", err);
		memset(read, 0, sizeof(*read));
		bap_long_op_reset();

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("handle 0x%04x", handle);

	if (data != NULL) {
		if (net_buf_simple_tailroom(&att_buf) < length) {
			LOG_DBG("Buffer full, invalid server response of size %u",
				length + att_buf.len);
			memset(read, 0, sizeof(*read));
			bap_long_op_reset();

			return BT_GATT_ITER_STOP;
		}

		/* store data*/
		net_buf_simple_add_mem(&att_buf, data, length);

		return BT_GATT_ITER_CONTINUE;
	}

	/* we reset the buffer so that it is ready for new data */
	memset(read, 0, sizeof(*read));
	data_length = att_buf.len;
	bap_long_op_reset();

	/* do the parse and callback to send  notify to application*/
	parse_and_send_recv_state(conn, handle,
				  att_buf.data, data_length, &recv_state);

	return BT_GATT_ITER_STOP;
}

static void long_bap_read(struct bt_conn *conn, uint16_t handle)
{
	int err;

	LOG_DBG("conn %p busy %u", conn, broadcast_assistant.busy);

	if (broadcast_assistant.busy) {
		/* If the client is busy reading or writing something else, reschedule the
		 * long read.
		 */
		struct bt_conn_info conn_info;

		err = bt_conn_get_info(conn, &conn_info);
		if (err != 0) {
			LOG_DBG("Failed to get conn info, use default interval");

			conn_info.le.interval = BT_GAP_INIT_CONN_INT_MIN;
		}

		/* Wait a connection interval to retry */
		err = k_work_reschedule(&broadcast_assistant.bap_read_work,
					K_USEC(BT_CONN_INTERVAL_TO_US(conn_info.le.interval)));
		if (err < 0) {
			LOG_DBG("Failed to reschedule read work: %d", err);
			bap_long_op_reset();
		}

		return;
	}

	broadcast_assistant.read_params.func = broadcast_assistant_bap_ntf_read_func;
	broadcast_assistant.read_params.handle_count = 1U;
	broadcast_assistant.read_params.single.handle = handle;
	broadcast_assistant.read_params.single.offset = att_buf.len;

	err = bt_gatt_read(conn, &broadcast_assistant.read_params);
	if (err != 0) {
		LOG_DBG("Failed to read: %d", err);
		bap_long_op_reset();
	} else {
		broadcast_assistant.busy = true;
	}
}

static void delayed_bap_read_handler(struct k_work *work)
{
	long_bap_read(broadcast_assistant.conn, broadcast_assistant.long_read_handle);
}

/** @brief Handles notifications and indications from the server */
static uint8_t notify_handler(struct bt_conn *conn,
			      struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_bap_scan_delegator_recv_state recv_state;
	int16_t index;

	if (data == NULL) {
		LOG_DBG("[UNSUBSCRIBED] %u", handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	LOG_HEXDUMP_DBG(data, length, "Receive state notification:");

	index = lookup_index_by_handle(handle);
	if (index < 0) {
		LOG_DBG("Invalid index");

		return BT_GATT_ITER_STOP;
	}

	if (length != 0) {
		const uint8_t att_ntf_header_size = 3; /* opcode (1) + handle (2) */
		uint16_t max_ntf_size;

		/* Cancel any pending long reads containing now obsolete information */
		(void)k_work_cancel_delayable(&broadcast_assistant.bap_read_work);

		if (conn != NULL) {
			max_ntf_size = bt_gatt_get_mtu(conn) - att_ntf_header_size;
		} else {
			max_ntf_size = MIN(BT_L2CAP_RX_MTU, BT_L2CAP_TX_MTU) - att_ntf_header_size;
		}

		if (length == max_ntf_size) {
			/* TODO: if we are busy we should not overwrite the long_read_handle,
			 * we'll have to keep track of the handle and parameters separately
			 * for each characteristic, similar to the bt_bap_unicast_client_ep
			 * struct for the unicast client
			 */
			broadcast_assistant.long_read_handle = handle;

			if (!broadcast_assistant.busy) {
				net_buf_simple_add_mem(&att_buf, data, length);
			}
			long_bap_read(conn, handle);
		} else {
			return parse_and_send_recv_state(conn, handle, data, length, &recv_state);
		}
	} else if (broadcast_assistant_cbs != NULL &&
		   broadcast_assistant_cbs->recv_state_removed != NULL) {
		broadcast_assistant.past_avail[index] = false;
		broadcast_assistant_cbs->recv_state_removed(conn, 0,
							    broadcast_assistant.src_ids[index]);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t read_recv_state_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	uint16_t handle = params->single.handle;
	uint8_t last_handle_index = broadcast_assistant.recv_state_cnt - 1;
	uint16_t last_handle = broadcast_assistant.recv_state_handles[last_handle_index];
	struct bt_bap_scan_delegator_recv_state recv_state;
	int cb_err = err;
	bool active_recv_state = data != NULL && length != 0;

	/* TODO: Split discovery and receive state characteristic read */

	(void)memset(params, 0, sizeof(*params));

	LOG_DBG("%s receive state", active_recv_state ? "Active " : "Inactive");

	if (cb_err == 0 && active_recv_state) {
		int16_t index;

		index = lookup_index_by_handle(handle);
		if (index < 0) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_HANDLE);
		} else {
			cb_err = parse_recv_state(data, length, &recv_state);

			if (cb_err != 0) {
				LOG_DBG("Invalid receive state");
			} else {
				broadcast_assistant.src_ids[index] = recv_state.src_id;
				broadcast_assistant.past_avail[index] =
					past_available(conn, &recv_state.addr,
						       recv_state.adv_sid);
			}
		}
	}

	if (cb_err != 0) {
		LOG_DBG("err: %d", cb_err);
		if (broadcast_assistant.discovering) {
			broadcast_assistant.discovering = false;
			if (broadcast_assistant_cbs != NULL &&
			    broadcast_assistant_cbs->discover != NULL) {
				broadcast_assistant_cbs->discover(conn,
								  cb_err, 0);
			}
		} else {
			if (broadcast_assistant_cbs != NULL &&
			    broadcast_assistant_cbs->recv_state != NULL) {
				broadcast_assistant_cbs->recv_state(conn,
								    cb_err,
								    NULL);
			}
		}
	} else if (handle == last_handle) {
		if (broadcast_assistant.discovering) {
			broadcast_assistant.discovering = false;
			if (broadcast_assistant_cbs != NULL &&
			    broadcast_assistant_cbs->discover != NULL) {
				broadcast_assistant_cbs->discover(
					conn, cb_err,
					broadcast_assistant.recv_state_cnt);
			}
		} else {
			if (broadcast_assistant_cbs != NULL &&
			    broadcast_assistant_cbs->recv_state != NULL) {
				if (active_recv_state) {
					broadcast_assistant_cbs->recv_state(conn,
									    cb_err,
									    &recv_state);
				} else {
					broadcast_assistant_cbs->recv_state(conn,
									    cb_err,
									    NULL);
				}
			}
		}
	} else {
		for (uint8_t i = 0U; i < broadcast_assistant.recv_state_cnt; i++) {
			if (handle == broadcast_assistant.recv_state_handles[i]) {
				if (i + 1 < ARRAY_SIZE(broadcast_assistant.recv_state_handles)) {
					(void)bt_bap_broadcast_assistant_read_recv_state(conn,
											 i + 1);
				}
				break;
			}
		}
	}

	return BT_GATT_ITER_STOP;
}

static void discover_init(void)
{
	(void)memset(&broadcast_assistant, 0, sizeof(broadcast_assistant));

	k_work_init_delayable(&broadcast_assistant.bap_read_work, delayed_bap_read_handler);

	net_buf_simple_reset(&att_buf);
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

	if (attr == NULL) {
		LOG_DBG("Found %u BASS receive states",
		       broadcast_assistant.recv_state_cnt);
		(void)memset(params, 0, sizeof(*params));

		err = bt_bap_broadcast_assistant_read_recv_state(conn, 0);
		if (err != 0) {
			broadcast_assistant.discovering = false;
			if (broadcast_assistant_cbs != NULL &&
			    broadcast_assistant_cbs->discover != NULL) {
				broadcast_assistant_cbs->discover(conn, err, 0);
			}
		}

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		struct bt_gatt_chrc *chrc =
			(struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_BASS_CONTROL_POINT) == 0) {
			LOG_DBG("Control Point");
			broadcast_assistant.cp_handle = attr->handle + 1;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_BASS_RECV_STATE) == 0) {
			if (broadcast_assistant.recv_state_cnt <
				CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT) {
				uint8_t idx = broadcast_assistant.recv_state_cnt++;

				LOG_DBG("Receive State %u", broadcast_assistant.recv_state_cnt);
				broadcast_assistant.recv_state_handles[idx] =
					attr->handle + 1;
				sub_params = &broadcast_assistant.recv_state_sub_params[idx];
				sub_params->disc_params =
					&broadcast_assistant.recv_state_disc_params[idx];
			}
		}

		if (sub_params != NULL) {
			/* With ccc_handle == 0 it will use auto discovery */
			sub_params->end_handle = broadcast_assistant.end_handle;
			sub_params->ccc_handle = 0;
			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = attr->handle + 1;
			sub_params->notify = notify_handler;
			err = bt_gatt_subscribe(conn, sub_params);

			if (err != 0 && err != -EALREADY) {
				LOG_DBG("Could not subscribe to handle 0x%04x: %d",
					sub_params->value_handle, err);

				broadcast_assistant.discovering = false;
				if (broadcast_assistant_cbs != NULL &&
				    broadcast_assistant_cbs->discover != NULL) {
					broadcast_assistant_cbs->discover(conn,
									  err,
									  0);
				}

				return BT_GATT_ITER_STOP;
			}
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t service_discover_func(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	int err;
	struct bt_gatt_service_val *prim_service;

	if (attr == NULL) {
		LOG_DBG("Could not discover BASS");
		(void)memset(params, 0, sizeof(*params));

		broadcast_assistant.discovering = false;

		if (broadcast_assistant_cbs != NULL &&
		    broadcast_assistant_cbs->discover != NULL) {
			err = BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
			broadcast_assistant_cbs->discover(conn, err, 0);
		}

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		broadcast_assistant.start_handle = attr->handle + 1;
		broadcast_assistant.end_handle = prim_service->end_handle;

		broadcast_assistant.disc_params.uuid = NULL;
		broadcast_assistant.disc_params.start_handle = broadcast_assistant.start_handle;
		broadcast_assistant.disc_params.end_handle = broadcast_assistant.end_handle;
		broadcast_assistant.disc_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		broadcast_assistant.disc_params.func = char_discover_func;

		err = bt_gatt_discover(conn, &broadcast_assistant.disc_params);
		if (err != 0) {
			LOG_DBG("Discover failed (err %d)", err);
			broadcast_assistant.discovering = false;

			if (broadcast_assistant_cbs != NULL &&
			    broadcast_assistant_cbs->discover != NULL) {
				broadcast_assistant_cbs->discover(conn, err, 0);
			}
		}
	}

	return BT_GATT_ITER_STOP;
}

static void bap_broadcast_assistant_write_cp_cb(struct bt_conn *conn, uint8_t err,
						struct bt_gatt_write_params *params)
{
	uint8_t opcode = net_buf_simple_pull_u8(&att_buf);

	broadcast_assistant.busy = false;

	if (broadcast_assistant_cbs == NULL) {
		return;
	}

	/* we reset the buffer, so that we are ready for new notifications and writes */
	net_buf_simple_reset(&att_buf);

	switch (opcode) {
	case BT_BAP_BASS_OP_SCAN_STOP:
		if (broadcast_assistant_cbs->scan_stop != NULL) {
			broadcast_assistant_cbs->scan_stop(conn, err);
		}
		break;
	case BT_BAP_BASS_OP_SCAN_START:
		if (broadcast_assistant_cbs->scan_start != NULL) {
			broadcast_assistant_cbs->scan_start(conn, err);
		}
		break;
	case BT_BAP_BASS_OP_ADD_SRC:
		if (broadcast_assistant_cbs->add_src != NULL) {
			broadcast_assistant_cbs->add_src(conn, err);
		}
		break;
	case BT_BAP_BASS_OP_MOD_SRC:
		if (broadcast_assistant_cbs->mod_src != NULL) {
			broadcast_assistant_cbs->mod_src(conn, err);
		}
		break;
	case BT_BAP_BASS_OP_BROADCAST_CODE:
		if (broadcast_assistant_cbs->broadcast_code != NULL) {
			broadcast_assistant_cbs->broadcast_code(conn, err);
		}
		break;
	case BT_BAP_BASS_OP_REM_SRC:
		if (broadcast_assistant_cbs->rem_src != NULL) {
			broadcast_assistant_cbs->rem_src(conn, err);
		}
		break;
	default:
		LOG_DBG("Unknown opcode 0x%02x", opcode);
		break;
	}
}

static int bt_bap_broadcast_assistant_common_cp(struct bt_conn *conn,
				    const struct net_buf_simple *buf)
{
	int err;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	} else if (broadcast_assistant.cp_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	broadcast_assistant.write_params.offset = 0;
	broadcast_assistant.write_params.data = buf->data;
	broadcast_assistant.write_params.length = buf->len;
	broadcast_assistant.write_params.handle = broadcast_assistant.cp_handle;
	broadcast_assistant.write_params.func = bap_broadcast_assistant_write_cp_cb;

	err = bt_gatt_write(conn, &broadcast_assistant.write_params);
	if (err == 0) {
		broadcast_assistant.busy = true;
	}

	return err;
}


static bool broadcast_source_found(struct bt_data *data, void *user_data)
{
	const struct bt_le_scan_recv_info *info = user_data;
	struct bt_uuid_16 adv_uuid;
	uint32_t broadcast_id;

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (data->data_len < BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO) != 0) {
		return true;
	}

	broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);

	LOG_DBG("Found BIS advertiser with address %s SID 0x%02X and broadcast_id 0x%06X",
		bt_addr_le_str(info->addr), info->sid, broadcast_id);

	if (broadcast_assistant_cbs != NULL &&
	    broadcast_assistant_cbs->scan != NULL) {
		broadcast_assistant_cbs->scan(info, broadcast_id);
	}

	return false;
}

void scan_recv(const struct bt_le_scan_recv_info *info,
	       struct net_buf_simple *ad)
{
	/* We are only interested in non-connectable periodic advertisers */
	if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0 ||
	    info->interval == 0) {
		return;
	}

	bt_data_parse(ad, broadcast_source_found, (void *)info);
}

static struct bt_le_scan_cb scan_cb = {
	.recv = scan_recv
};

/****************************** PUBLIC API ******************************/

static int broadcast_assistant_reset(struct bap_broadcast_assistant_instance *inst)
{
	broadcast_assistant.long_read_handle = 0;
	(void)k_work_cancel_delayable(&broadcast_assistant.bap_read_work);

	if (inst->conn != NULL) {
		struct bt_conn *conn = inst->conn;

		bt_conn_unref(conn);
		inst->conn = NULL;
	}

	return 0;
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	(void)broadcast_assistant_reset(&broadcast_assistant);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected_cb,
};

int bt_bap_broadcast_assistant_discover(struct bt_conn *conn)
{
	int err;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	if (broadcast_assistant.busy) {
		LOG_DBG("Instance is busy");
		return -EBUSY;
	}

	err = broadcast_assistant_reset(&broadcast_assistant);
	if (err != 0) {
		LOG_DBG("Failed to reset broadcast assistant: %d", err);

		return err;
	}

	/* Discover BASS on peer, setup handles and notify */
	discover_init();

	(void)memcpy(&uuid, BT_UUID_BASS, sizeof(uuid));
	broadcast_assistant.disc_params.func = service_discover_func;
	broadcast_assistant.disc_params.uuid = &uuid.uuid;
	broadcast_assistant.disc_params.type = BT_GATT_DISCOVER_PRIMARY;
	broadcast_assistant.disc_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	broadcast_assistant.disc_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	err = bt_gatt_discover(conn, &broadcast_assistant.disc_params);
	if (err != 0) {
		return err;
	}

	broadcast_assistant.discovering = true;
	broadcast_assistant.conn = bt_conn_ref(conn);

	return 0;
}

void bt_bap_broadcast_assistant_register_cb(struct bt_bap_broadcast_assistant_cb *cb)
{
	broadcast_assistant_cbs = cb;
}

int bt_bap_broadcast_assistant_scan_start(struct bt_conn *conn, bool start_scan)
{
	struct bt_bap_bass_cp_scan_start *cp;
	int err;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	} else if (broadcast_assistant.cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (broadcast_assistant.busy) {
		LOG_DBG("instance busy");

		return -EBUSY;
	}

	if (start_scan) {
		static bool cb_registered;

		if (!cb_registered) {
			bt_le_scan_cb_register(&scan_cb);
			cb_registered = true;
		}

		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
		if (err != 0) {
			LOG_DBG("Could not start scan (%d)", err);

			return err;
		}

		broadcast_assistant.scanning = true;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&att_buf);
	cp = net_buf_simple_add(&att_buf, sizeof(*cp));

	cp->opcode = BT_BAP_BASS_OP_SCAN_START;

	return bt_bap_broadcast_assistant_common_cp(conn, &att_buf);
}

int bt_bap_broadcast_assistant_scan_stop(struct bt_conn *conn)
{
	struct bt_bap_bass_cp_scan_stop *cp;
	int err;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	} else if (broadcast_assistant.cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (broadcast_assistant.busy) {
		LOG_DBG("instance busy");

		return -EBUSY;
	}

	if (broadcast_assistant.scanning) {
		err = bt_le_scan_stop();
		if (err != 0) {
			LOG_DBG("Could not stop scan (%d)", err);

			return err;
		}

		broadcast_assistant.scanning = false;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&att_buf);
	cp = net_buf_simple_add(&att_buf, sizeof(*cp));

	cp->opcode = BT_BAP_BASS_OP_SCAN_STOP;

	return bt_bap_broadcast_assistant_common_cp(conn, &att_buf);
}

int bt_bap_broadcast_assistant_add_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_add_src_param *param)
{
	struct bt_bap_bass_cp_add_src *cp;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	} else if (broadcast_assistant.cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (broadcast_assistant.busy) {
		LOG_DBG("instance busy");

		return -EBUSY;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&att_buf);
	cp = net_buf_simple_add(&att_buf, sizeof(*cp));

	cp->opcode = BT_BAP_BASS_OP_ADD_SRC;
	cp->adv_sid = param->adv_sid;
	bt_addr_le_copy(&cp->addr, &param->addr);

	sys_put_le24(param->broadcast_id, cp->broadcast_id);

	if (param->pa_sync) {
		if (past_available(conn, &param->addr, param->adv_sid)) {
			cp->pa_sync = BT_BAP_BASS_PA_REQ_SYNC_PAST;
		} else {
			cp->pa_sync = BT_BAP_BASS_PA_REQ_SYNC;
		}
	} else {
		cp->pa_sync = BT_BAP_BASS_PA_REQ_NO_SYNC;
	}
	cp->pa_interval = sys_cpu_to_le16(param->pa_interval);

	cp->num_subgroups = param->num_subgroups;
	for (int i = 0; i < param->num_subgroups; i++) {
		struct bt_bap_bass_cp_subgroup *subgroup;
		const size_t subgroup_size = sizeof(subgroup->bis_sync) +
					     sizeof(subgroup->metadata_len) +
					     param->subgroups[i].metadata_len;

		if (att_buf.len + subgroup_size > att_buf.size) {
			LOG_DBG("MTU is too small to send %zu octets", att_buf.len + subgroup_size);

			return -EINVAL;
		}

		subgroup = net_buf_simple_add(&att_buf, subgroup_size);

		/* The BIS Index bitfield to be sent must use BIT(0) for BIS Index 1 */
		subgroup->bis_sync = param->subgroups[i].bis_sync >> 1;

		CHECKIF(param->pa_sync == 0 && subgroup->bis_sync != 0) {
			LOG_DBG("Only syncing to BIS is not allowed");
			return -EINVAL;
		}

		if (param->subgroups[i].metadata_len != 0) {
			(void)memcpy(subgroup->metadata,
				     param->subgroups[i].metadata,
				     param->subgroups[i].metadata_len);
			subgroup->metadata_len = param->subgroups[i].metadata_len;
		} else {
			subgroup->metadata_len = 0;
		}

	}

	return bt_bap_broadcast_assistant_common_cp(conn, &att_buf);
}

int bt_bap_broadcast_assistant_mod_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_mod_src_param *param)
{
	struct bt_bap_bass_cp_mod_src *cp;
	bool known_recv_state;
	bool past_avail;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	} else if (broadcast_assistant.cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (broadcast_assistant.busy) {
		LOG_DBG("instance busy");

		return -EBUSY;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&att_buf);
	cp = net_buf_simple_add(&att_buf, sizeof(*cp));

	cp->opcode = BT_BAP_BASS_OP_MOD_SRC;
	cp->src_id = param->src_id;

	/* Since we do not have the address and SID in this function, we
	 * rely on cached PAST support information
	 */
	known_recv_state = false;
	past_avail = false;
	for (size_t i = 0; i < ARRAY_SIZE(broadcast_assistant.src_ids); i++) {
		if (broadcast_assistant.src_ids[i] == param->src_id) {
			known_recv_state = true;
			past_avail = broadcast_assistant.past_avail[i];
			break;
		}
	}

	if (!known_recv_state) {
		LOG_WRN("Attempting to modify unknown receive state: %u",
			param->src_id);
	}

	if (param->pa_sync) {
		if (known_recv_state && past_avail) {
			cp->pa_sync = BT_BAP_BASS_PA_REQ_SYNC_PAST;
		} else {
			cp->pa_sync = BT_BAP_BASS_PA_REQ_SYNC;
		}
	} else {
		cp->pa_sync = BT_BAP_BASS_PA_REQ_NO_SYNC;
	}

	cp->pa_interval = sys_cpu_to_le16(param->pa_interval);

	cp->num_subgroups = param->num_subgroups;
	for (int i = 0; i < param->num_subgroups; i++) {
		struct bt_bap_bass_cp_subgroup *subgroup;
		const size_t subgroup_size = sizeof(subgroup->bis_sync) +
					     sizeof(subgroup->metadata_len) +
					     param->subgroups[i].metadata_len;

		if (att_buf.len + subgroup_size > att_buf.size) {
			LOG_DBG("MTU is too small to send %zu octets", att_buf.len + subgroup_size);
			return -EINVAL;
		}
		subgroup = net_buf_simple_add(&att_buf, subgroup_size);

		/* The BIS Index bitfield to be sent must use BIT(0) for BIS Index 1 */
		subgroup->bis_sync = param->subgroups[i].bis_sync >> 1;

		CHECKIF(param->pa_sync == 0 && subgroup->bis_sync != 0) {
			LOG_DBG("Only syncing to BIS is not allowed");
			return -EINVAL;
		}

		if (param->subgroups[i].metadata_len != 0) {
			(void)memcpy(subgroup->metadata,
				     param->subgroups[i].metadata,
				     param->subgroups[i].metadata_len);
			subgroup->metadata_len = param->subgroups[i].metadata_len;
		} else {
			subgroup->metadata_len = 0;
		}
	}

	return bt_bap_broadcast_assistant_common_cp(conn, &att_buf);
}

int bt_bap_broadcast_assistant_set_broadcast_code(
	struct bt_conn *conn, uint8_t src_id,
	const uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE])
{
	struct bt_bap_bass_cp_broadcase_code *cp;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	} else if (broadcast_assistant.cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (broadcast_assistant.busy) {
		LOG_DBG("instance busy");

		return -EBUSY;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&att_buf);
	cp = net_buf_simple_add(&att_buf, sizeof(*cp));

	cp->opcode = BT_BAP_BASS_OP_BROADCAST_CODE;
	cp->src_id = src_id;

	(void)memcpy(cp->broadcast_code, broadcast_code,
		     BT_AUDIO_BROADCAST_CODE_SIZE);

	LOG_HEXDUMP_DBG(cp->broadcast_code, BT_AUDIO_BROADCAST_CODE_SIZE, "broadcast code:");

	return bt_bap_broadcast_assistant_common_cp(conn, &att_buf);
}

int bt_bap_broadcast_assistant_rem_src(struct bt_conn *conn, uint8_t src_id)
{
	struct bt_bap_bass_cp_rem_src *cp;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	} else if (broadcast_assistant.cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (broadcast_assistant.busy) {
		LOG_DBG("instance busy");

		return -EBUSY;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&att_buf);
	cp = net_buf_simple_add(&att_buf, sizeof(*cp));

	cp->opcode = BT_BAP_BASS_OP_REM_SRC;
	cp->src_id = src_id;

	return bt_bap_broadcast_assistant_common_cp(conn, &att_buf);
}

int bt_bap_broadcast_assistant_read_recv_state(struct bt_conn *conn,
					       uint8_t idx)
{
	int err;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	CHECKIF(idx >= ARRAY_SIZE(broadcast_assistant.recv_state_handles)) {
		LOG_DBG("Invalid idx: %u", idx);

		return -EINVAL;
	}

	if (broadcast_assistant.recv_state_handles[idx] == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	}

	broadcast_assistant.read_params.func = read_recv_state_cb;
	broadcast_assistant.read_params.handle_count = 1;
	broadcast_assistant.read_params.single.handle =
		broadcast_assistant.recv_state_handles[idx];

	err = bt_gatt_read(conn, &broadcast_assistant.read_params);
	if (err != 0) {
		(void)memset(&broadcast_assistant.read_params, 0,
			     sizeof(broadcast_assistant.read_params));
	}

	return err;
}
