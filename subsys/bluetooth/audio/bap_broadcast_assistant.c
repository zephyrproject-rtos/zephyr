/*  Bluetooth BAP Broadcast Assistant */

/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

#include <zephyr/logging/log.h>
#include <sys/errno.h>

LOG_MODULE_REGISTER(bt_bap_broadcast_assistant, CONFIG_BT_BAP_BROADCAST_ASSISTANT_LOG_LEVEL);

#include "common/bt_str.h"

#include "audio_internal.h"
#include "bap_internal.h"

#define MINIMUM_RECV_STATE_LEN          15

struct bap_broadcast_assistant_recv_state_info {
	uint8_t src_id;
	/** Cached PAST available */
	bool past_avail;
	uint8_t adv_sid;
	uint32_t broadcast_id;
	bt_addr_le_t addr;
};

enum bap_broadcast_assistant_flag {
	BAP_BA_FLAG_BUSY,
	BAP_BA_FLAG_DISCOVER_IN_PROGRESS,
	BAP_BA_FLAG_SCANNING,

	BAP_BA_FLAG_NUM_FLAGS, /* keep as last */
};

struct bap_broadcast_assistant_instance {
	struct bt_conn *conn;
	bool scanning;
	uint8_t pa_sync;
	uint8_t recv_state_cnt;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t cp_handle;
	uint16_t recv_state_handles[CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT];

	struct bt_gatt_subscribe_params recv_state_sub_params
		[CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT];
	struct bt_gatt_discover_params
		recv_state_disc_params[CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT];

	/* We ever only allow a single outstanding operation per instance, so we can resuse the
	 * memory for the GATT params
	 */
	union {
		struct bt_gatt_read_params read_params;
		struct bt_gatt_write_params write_params;
		struct bt_gatt_discover_params disc_params;
	};

	struct k_work_delayable bap_read_work;
	uint16_t long_read_handle;

	struct bap_broadcast_assistant_recv_state_info
		recv_states[CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT];

	ATOMIC_DEFINE(flags, BAP_BA_FLAG_NUM_FLAGS);
};

static sys_slist_t broadcast_assistant_cbs = SYS_SLIST_STATIC_INIT(&broadcast_assistant_cbs);

static struct bap_broadcast_assistant_instance broadcast_assistants[CONFIG_BT_MAX_CONN];
static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

#define ATT_BUF_SIZE BT_ATT_MAX_ATTRIBUTE_LEN
NET_BUF_SIMPLE_DEFINE_STATIC(att_buf, ATT_BUF_SIZE);

static int read_recv_state(struct bap_broadcast_assistant_instance *inst, uint8_t idx);

static int16_t lookup_index_by_handle(struct bap_broadcast_assistant_instance *inst,
				      uint16_t handle)
{
	for (size_t i = 0U; i < ARRAY_SIZE(inst->recv_state_handles); i++) {
		if (inst->recv_state_handles[i] == handle) {
			return i;
		}
	}

	LOG_ERR("Unknown handle 0x%04x", handle);

	return -1;
}

static struct bap_broadcast_assistant_instance *inst_by_conn(struct bt_conn *conn)
{
	struct bap_broadcast_assistant_instance *inst;

	if (conn == NULL) {
		LOG_DBG("NULL conn");
		return NULL;
	}

	inst = &broadcast_assistants[bt_conn_index(conn)];

	if (inst->conn == conn) {
		return inst;
	}

	return NULL;
}

static void bap_broadcast_assistant_discover_complete(struct bt_conn *conn, int err,
						      uint8_t recv_state_count)
{
	struct bap_broadcast_assistant_instance *inst = inst_by_conn(conn);
	struct bt_bap_broadcast_assistant_cb *listener, *next;

	net_buf_simple_reset(&att_buf);
	if (inst != NULL) {
		atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);
		atomic_clear_bit(inst->flags, BAP_BA_FLAG_DISCOVER_IN_PROGRESS);
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&broadcast_assistant_cbs,
					  listener, next, _node) {
		if (listener->discover) {
			listener->discover(conn, err, recv_state_count);
		}
	}
}

static void bap_broadcast_assistant_recv_state_changed(
	struct bt_conn *conn, int err, const struct bt_bap_scan_delegator_recv_state *state)
{
	struct bt_bap_broadcast_assistant_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&broadcast_assistant_cbs,
					  listener, next, _node) {
		if (listener->recv_state) {
			listener->recv_state(conn, err, state);
		}
	}
}

static void bap_broadcast_assistant_recv_state_removed(struct bt_conn *conn, uint8_t src_id)
{
	struct bt_bap_broadcast_assistant_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&broadcast_assistant_cbs,
					  listener, next, _node) {
		if (listener->recv_state_removed) {
			listener->recv_state_removed(conn, src_id);
		}
	}
}

static void bap_broadcast_assistant_scan_results(const struct bt_le_scan_recv_info *info,
						 uint32_t broadcast_id)
{
	struct bt_bap_broadcast_assistant_cb *listener, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&broadcast_assistant_cbs, listener, next, _node) {
		if (listener->scan) {
			listener->scan(info, broadcast_id);
		}
	}
}

static bool past_available(const struct bt_conn *conn,
			   const bt_addr_le_t *adv_addr,
			   uint8_t sid)
{
	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER)) {
		struct bt_le_local_features local_features;
		struct bt_conn_remote_info remote_info;
		int err;

		err = bt_le_get_local_features(&local_features);
		if (err != 0) {
			LOG_DBG("Failed to get local features: %d", err);
			return false;
		}

		err = bt_conn_get_remote_info(conn, &remote_info);
		if (err != 0) {
			LOG_DBG("Failed to get remote info: %d", err);
			return false;
		}

		LOG_DBG("%p remote %s PAST, local %s PAST", (void *)conn,
			BT_FEAT_LE_PAST_RECV(remote_info.le.features) ? "supports"
								      : "does not support",
			BT_FEAT_LE_PAST_SEND(local_features.features) ? "supports"
								      : "does not support");

		return BT_FEAT_LE_PAST_RECV(remote_info.le.features) &&
		       BT_FEAT_LE_PAST_SEND(local_features.features) &&
		       bt_le_per_adv_sync_lookup_addr(adv_addr, sid) != NULL;
	} else {
		return false;
	}
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

		broadcast_code = net_buf_simple_pull_mem(&buf, BT_ISO_BROADCAST_CODE_SIZE);
		(void)memcpy(recv_state->bad_code, broadcast_code,
			     sizeof(recv_state->bad_code));
	}

	recv_state->num_subgroups = net_buf_simple_pull_u8(&buf);
	if (recv_state->num_subgroups > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		LOG_DBG("Cannot parse %u subgroups (max %d)", recv_state->num_subgroups,
			CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);

		return -ENOMEM;
	}

	for (int i = 0; i < recv_state->num_subgroups; i++) {
		struct bt_bap_bass_subgroup *subgroup = &recv_state->subgroups[i];
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

static void bap_long_read_reset(struct bap_broadcast_assistant_instance *inst)
{
	inst->long_read_handle = 0;
	net_buf_simple_reset(&att_buf);
	atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);
}

static uint8_t parse_and_send_recv_state(struct bt_conn *conn, uint16_t handle,
					 const void *data, uint16_t length,
					 struct bt_bap_scan_delegator_recv_state *recv_state)
{
	int err;
	int16_t index;
	struct bap_broadcast_assistant_instance *inst = inst_by_conn(conn);

	if (inst == NULL) {
		return BT_GATT_ITER_STOP;
	}

	err = parse_recv_state(data, length, recv_state);
	if (err != 0) {
		LOG_WRN("Invalid receive state received");

		return BT_GATT_ITER_STOP;
	}

	index = lookup_index_by_handle(inst, handle);
	if (index < 0) {
		LOG_DBG("Invalid index");

		return BT_GATT_ITER_STOP;
	}

	inst->recv_states[index].src_id = recv_state->src_id;
	inst->recv_states[index].past_avail = past_available(conn, &recv_state->addr,
							     recv_state->adv_sid);

	bap_broadcast_assistant_recv_state_changed(conn, 0, recv_state);

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t broadcast_assistant_bap_ntf_read_func(struct bt_conn *conn, uint8_t err,
						     struct bt_gatt_read_params *read,
						     const void *data, uint16_t length)
{
	struct bt_bap_scan_delegator_recv_state recv_state;
	uint16_t handle = read->single.handle;
	uint16_t data_length;
	struct bap_broadcast_assistant_instance *inst = inst_by_conn(conn);

	if (inst == NULL) {
		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("conn %p err 0x%02x len %u", (void *)conn, err, length);

	if (err) {
		LOG_DBG("Failed to read: %u", err);
		memset(read, 0, sizeof(*read));
		bap_long_read_reset(inst);

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("handle 0x%04x", handle);

	if (data != NULL) {
		if (net_buf_simple_tailroom(&att_buf) < length) {
			LOG_DBG("Buffer full, invalid server response of size %u",
				length + att_buf.len);
			memset(read, 0, sizeof(*read));
			bap_long_read_reset(inst);

			return BT_GATT_ITER_STOP;
		}

		/* store data*/
		net_buf_simple_add_mem(&att_buf, data, length);

		return BT_GATT_ITER_CONTINUE;
	}

	/* we reset the buffer so that it is ready for new data */
	memset(read, 0, sizeof(*read));
	data_length = att_buf.len;
	bap_long_read_reset(inst);

	/* do the parse and callback to send  notify to application*/
	parse_and_send_recv_state(conn, handle, att_buf.data, data_length, &recv_state);

	return BT_GATT_ITER_STOP;
}

static void long_bap_read(struct bt_conn *conn, uint16_t handle)
{
	int err;
	struct bap_broadcast_assistant_instance *inst = inst_by_conn(conn);

	if (inst == NULL) {
		return;
	}

	if (atomic_test_and_set_bit(inst->flags, BAP_BA_FLAG_BUSY)) {
		LOG_DBG("conn %p", (void *)conn);

		/* If the client is busy reading reschedule the long read */
		struct bt_conn_info conn_info;

		err = bt_conn_get_info(conn, &conn_info);
		if (err != 0) {
			LOG_DBG("Failed to get conn info, use default interval");

			conn_info.le.interval = BT_GAP_INIT_CONN_INT_MIN;
		}

		/* Wait a connection interval to retry */
		err = k_work_reschedule(&inst->bap_read_work,
					K_USEC(BT_CONN_INTERVAL_TO_US(conn_info.le.interval)));
		if (err < 0) {
			LOG_DBG("Failed to reschedule read work: %d", err);
			bap_long_read_reset(inst);
		}

		return;
	}

	inst->read_params.func = broadcast_assistant_bap_ntf_read_func;
	inst->read_params.handle_count = 1U;
	inst->read_params.single.handle = handle;
	inst->read_params.single.offset = att_buf.len;

	err = bt_gatt_read(conn, &inst->read_params);
	if (err != 0) {
		/* TODO: If read failed due to buffers, retry */
		LOG_DBG("Failed to read: %d", err);
		bap_long_read_reset(inst);
	}
}

static void delayed_bap_read_handler(struct k_work *work)
{
	struct bap_broadcast_assistant_instance *inst =
		CONTAINER_OF((struct k_work_delayable *)work,
			     struct bap_broadcast_assistant_instance, bap_read_work);
	long_bap_read(inst->conn, inst->long_read_handle);
}

/** @brief Handles notifications and indications from the server */
static uint8_t notify_handler(struct bt_conn *conn,
			      struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	uint16_t handle = params->value_handle;
	struct bt_bap_scan_delegator_recv_state recv_state;
	int16_t index;
	struct bap_broadcast_assistant_instance *inst;

	if (conn == NULL) {
		/* Indicates that the CCC has been removed - no-op */
		return BT_GATT_ITER_CONTINUE;
	}

	inst = inst_by_conn(conn);

	if (inst == NULL) {
		return BT_GATT_ITER_STOP;
	}

	if (atomic_test_bit(inst->flags, BAP_BA_FLAG_DISCOVER_IN_PROGRESS)) {
		/* If we are discovering then we ignore notifications as the handles may change */
		return BT_GATT_ITER_CONTINUE;
	}

	if (data == NULL) {
		LOG_DBG("[UNSUBSCRIBED] %u", handle);
		params->value_handle = 0U;

		return BT_GATT_ITER_STOP;
	}

	LOG_HEXDUMP_DBG(data, length, "Receive state notification:");

	index = lookup_index_by_handle(inst, handle);
	if (index < 0) {
		LOG_DBG("Invalid index");

		return BT_GATT_ITER_STOP;
	}

	if (length != 0) {
		const uint16_t max_ntf_size = bt_audio_get_max_ntf_size(conn);

		/* Cancel any pending long reads containing now obsolete information */
		(void)k_work_cancel_delayable(&inst->bap_read_work);

		if (length == max_ntf_size) {
			/* TODO: if we are busy we should not overwrite the long_read_handle,
			 * we'll have to keep track of the handle and parameters separately
			 * for each characteristic, similar to the bt_bap_unicast_client_ep
			 * struct for the unicast client
			 */
			inst->long_read_handle = handle;

			if (!atomic_test_bit(inst->flags, BAP_BA_FLAG_BUSY)) {
				net_buf_simple_add_mem(&att_buf, data, length);
			}

			long_bap_read(conn, handle);
		} else {
			return parse_and_send_recv_state(conn, handle, data, length, &recv_state);
		}
	} else {
		inst->recv_states[index].past_avail = false;
		bap_broadcast_assistant_recv_state_removed(conn, inst->recv_states[index].src_id);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t read_recv_state_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	struct bap_broadcast_assistant_instance *inst = inst_by_conn(conn);

	if (inst == NULL) {
		return BT_GATT_ITER_STOP;
	}

	uint16_t handle = params->single.handle;
	uint8_t last_handle_index = inst->recv_state_cnt - 1;
	uint16_t last_handle = inst->recv_state_handles[last_handle_index];
	struct bt_bap_scan_delegator_recv_state recv_state;
	int cb_err = err;
	bool active_recv_state = data != NULL && length != 0;

	/* TODO: Split discovery and receive state characteristic read */

	(void)memset(params, 0, sizeof(*params));

	LOG_DBG("%s receive state", active_recv_state ? "Active " : "Inactive");

	if (cb_err == 0 && active_recv_state) {
		int16_t index;

		index = lookup_index_by_handle(inst, handle);
		if (index < 0) {
			cb_err = BT_GATT_ERR(BT_ATT_ERR_INVALID_HANDLE);
		} else {
			cb_err = parse_recv_state(data, length, &recv_state);

			if (cb_err != 0) {
				LOG_DBG("Invalid receive state");
			} else {
				struct bap_broadcast_assistant_recv_state_info *stored_state =
					&inst->recv_states[index];

				stored_state->src_id = recv_state.src_id;
				stored_state->adv_sid = recv_state.adv_sid;
				stored_state->broadcast_id = recv_state.broadcast_id;
				bt_addr_le_copy(&stored_state->addr, &recv_state.addr);
				inst->recv_states[index].past_avail =
					past_available(conn, &recv_state.addr,
						       recv_state.adv_sid);
			}
		}
	}

	if (cb_err != 0) {
		LOG_DBG("err %d", cb_err);

		if (atomic_test_bit(inst->flags, BAP_BA_FLAG_DISCOVER_IN_PROGRESS)) {
			bap_broadcast_assistant_discover_complete(conn, cb_err, 0);
		} else {
			atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);
			bap_broadcast_assistant_recv_state_changed(conn, cb_err, NULL);
		}
	} else if (handle == last_handle) {
		if (atomic_test_bit(inst->flags, BAP_BA_FLAG_DISCOVER_IN_PROGRESS)) {
			const uint8_t recv_state_cnt = inst->recv_state_cnt;

			bap_broadcast_assistant_discover_complete(conn, cb_err, recv_state_cnt);
		} else {
			atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);
			bap_broadcast_assistant_recv_state_changed(conn, cb_err,
								   active_recv_state ?
								   &recv_state : NULL);
		}
	} else {
		for (uint8_t i = 0U; i < inst->recv_state_cnt; i++) {
			if (handle != inst->recv_state_handles[i]) {
				continue;
			}

			if (i + 1 < ARRAY_SIZE(inst->recv_state_handles)) {
				cb_err = read_recv_state(inst, i + 1);
				if (cb_err != 0) {
					LOG_DBG("Failed to read receive state: %d", cb_err);

					if (atomic_test_bit(inst->flags,
							    BAP_BA_FLAG_DISCOVER_IN_PROGRESS)) {
						bap_broadcast_assistant_discover_complete(
							conn, cb_err, 0);
					} else {
						atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);
						bap_broadcast_assistant_recv_state_changed(
							conn, cb_err, NULL);
					}
				}
			}
			break;
		}
	}

	return BT_GATT_ITER_STOP;
}

static void discover_init(struct bap_broadcast_assistant_instance *inst)
{
	k_work_init_delayable(&inst->bap_read_work, delayed_bap_read_handler);
	net_buf_simple_reset(&att_buf);
	atomic_set_bit(inst->flags, BAP_BA_FLAG_DISCOVER_IN_PROGRESS);
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
	struct bap_broadcast_assistant_instance *inst = inst_by_conn(conn);

	if (inst == NULL) {
		return BT_GATT_ITER_STOP;
	}

	if (attr == NULL) {
		LOG_DBG("Found %u BASS receive states", inst->recv_state_cnt);
		(void)memset(params, 0, sizeof(*params));

		err = read_recv_state(inst, 0);
		if (err != 0) {
			bap_broadcast_assistant_discover_complete(conn, err, 0);
		}

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		struct bt_gatt_chrc *chrc =
			(struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_BASS_CONTROL_POINT) == 0) {
			LOG_DBG("Control Point");
			inst->cp_handle = attr->handle + 1;
		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_BASS_RECV_STATE) == 0) {
			if (inst->recv_state_cnt <
				CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT) {
				const uint8_t idx = inst->recv_state_cnt;

				LOG_DBG("Receive State %u", inst->recv_state_cnt);
				inst->recv_state_handles[idx] =
					attr->handle + 1;
				sub_params = &inst->recv_state_sub_params[idx];
				sub_params->disc_params = &inst->recv_state_disc_params[idx];
				inst->recv_state_cnt++;
			}
		} else {
			LOG_DBG("Invalid UUID %s", bt_uuid_str(chrc->uuid));
			bap_broadcast_assistant_discover_complete(conn, -EBADMSG, 0);

			return BT_GATT_ITER_STOP;
		}

		if (sub_params != NULL) {
			sub_params->end_handle = inst->end_handle;
			sub_params->ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
			sub_params->value = BT_GATT_CCC_NOTIFY;
			sub_params->value_handle = attr->handle + 1;
			sub_params->notify = notify_handler;
			atomic_set_bit(sub_params->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

			err = bt_gatt_subscribe(conn, sub_params);
			if (err != 0) {
				LOG_DBG("Could not subscribe to handle 0x%04x: %d",
					sub_params->value_handle, err);

				bap_broadcast_assistant_discover_complete(conn, err, 0);

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
	struct bap_broadcast_assistant_instance *inst = inst_by_conn(conn);

	if (inst == NULL) {
		return BT_GATT_ITER_STOP;
	}

	if (attr == NULL) {
		LOG_DBG("Could not discover BASS");
		(void)memset(params, 0, sizeof(*params));

		err = BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);

		bap_broadcast_assistant_discover_complete(conn, err, 0);

		return BT_GATT_ITER_STOP;
	}

	LOG_DBG("[ATTRIBUTE] handle 0x%04X", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		prim_service = (struct bt_gatt_service_val *)attr->user_data;
		inst->start_handle = attr->handle + 1;
		inst->end_handle = prim_service->end_handle;

		inst->disc_params.uuid = NULL;
		inst->disc_params.start_handle = inst->start_handle;
		inst->disc_params.end_handle = inst->end_handle;
		inst->disc_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		inst->disc_params.func = char_discover_func;

		err = bt_gatt_discover(conn, &inst->disc_params);
		if (err != 0) {
			LOG_DBG("Discover failed (err %d)", err);
			bap_broadcast_assistant_discover_complete(conn, err, 0);
		}
	}

	return BT_GATT_ITER_STOP;
}

static void bap_broadcast_assistant_write_cp_cb(struct bt_conn *conn, uint8_t err,
						struct bt_gatt_write_params *params)
{
	struct bt_bap_broadcast_assistant_cb *listener, *next;
	uint8_t opcode = net_buf_simple_pull_u8(&att_buf);
	struct bap_broadcast_assistant_instance *inst = inst_by_conn(conn);

	if (inst == NULL) {
		return;
	}

	/* we reset the buffer, so that we are ready for new notifications and writes */
	net_buf_simple_reset(&att_buf);

	atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&broadcast_assistant_cbs, listener, next, _node) {
		switch (opcode) {
		case BT_BAP_BASS_OP_SCAN_STOP:
			if (listener->scan_stop != NULL) {
				listener->scan_stop(conn, err);
			}
			break;
		case BT_BAP_BASS_OP_SCAN_START:
			if (listener->scan_start != NULL) {
				listener->scan_start(conn, err);
			}
			break;
		case BT_BAP_BASS_OP_ADD_SRC:
			if (listener->add_src != NULL) {
				listener->add_src(conn, err);
			}
			break;
		case BT_BAP_BASS_OP_MOD_SRC:
			if (listener->mod_src != NULL) {
				listener->mod_src(conn, err);
			}
			break;
		case BT_BAP_BASS_OP_BROADCAST_CODE:
			if (listener->broadcast_code != NULL) {
				listener->broadcast_code(conn, err);
			}
			break;
		case BT_BAP_BASS_OP_REM_SRC:
			if (listener->rem_src != NULL) {
				listener->rem_src(conn, err);
			}
			break;
		default:
			LOG_DBG("Unknown opcode 0x%02x", opcode);
			break;
		}
	}
}

static int bt_bap_broadcast_assistant_common_cp(struct bt_conn *conn,
				    const struct net_buf_simple *buf)
{
	int err;
	struct bap_broadcast_assistant_instance *inst;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	inst = inst_by_conn(conn);

	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->cp_handle == 0) {
		LOG_DBG("Handle not set");
		return -EINVAL;
	}

	inst->write_params.offset = 0;
	inst->write_params.data = buf->data;
	inst->write_params.length = buf->len;
	inst->write_params.handle = inst->cp_handle;
	inst->write_params.func = bap_broadcast_assistant_write_cp_cb;

	err = bt_gatt_write(conn, &inst->write_params);
	if (err != 0) {
		atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);

		/* Report expected possible errors */
		if (err == -ENOTCONN || err == -ENOMEM) {
			return err;
		}

		return -ENOEXEC;
	}

	return 0;
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

	bap_broadcast_assistant_scan_results(info, broadcast_id);

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

/* BAP 6.5.4 states that the Broadcast Assistant shall not initiate the Add Source operation
 * if the operation would result in duplicate values for the combined Source_Address_Type,
 * Source_Adv_SID, and Broadcast_ID fields of any Broadcast Receive State characteristic exposed
 * by the Scan Delegator.
 */
static bool broadcast_src_is_duplicate(struct bap_broadcast_assistant_instance *inst,
				       uint32_t broadcast_id, uint8_t adv_sid, uint8_t addr_type)
{
	for (size_t i = 0; i < ARRAY_SIZE(inst->recv_states); i++) {
		const struct bap_broadcast_assistant_recv_state_info *state =
							&inst->recv_states[i];

		if (state != NULL && state->broadcast_id == broadcast_id &&
			state->adv_sid == adv_sid && state->addr.type == addr_type) {
			LOG_DBG("recv_state already exists at src_id=0x%02X", state->src_id);

			return true;
		}
	}

	return false;
}

/****************************** PUBLIC API ******************************/

static int broadcast_assistant_reset(struct bap_broadcast_assistant_instance *inst)
{
	inst->scanning = false;
	inst->pa_sync = 0U;
	inst->recv_state_cnt = 0U;
	inst->start_handle = 0U;
	inst->end_handle = 0U;
	inst->cp_handle = 0U;
	inst->long_read_handle = 0;
	(void)k_work_cancel_delayable(&inst->bap_read_work);

	for (int i = 0U; i < CONFIG_BT_BAP_BROADCAST_ASSISTANT_RECV_STATE_COUNT; i++) {
		memset(&inst->recv_states[i], 0, sizeof(inst->recv_states[i]));
		inst->recv_states[i].broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
		inst->recv_states[i].adv_sid = BT_HCI_LE_EXT_ADV_SID_INVALID;
		inst->recv_states[i].past_avail = false;
		inst->recv_state_handles[i] = 0U;
	}

	if (inst->conn != NULL) {
		struct bt_conn *conn = inst->conn;
		struct bt_conn_info info;
		int err;

		err = bt_conn_get_info(conn, &info);
		if (err != 0) {
			return err;
		}

		if (info.state == BT_CONN_STATE_CONNECTED) {
			for (size_t i = 0U; i < ARRAY_SIZE(inst->recv_state_sub_params); i++) {
				/* It's okay if this fail with -EINVAL as that means that they are
				 * not currently subscribed
				 */
				err = bt_gatt_unsubscribe(conn, &inst->recv_state_sub_params[i]);
				if (err != 0 && err != -EINVAL) {
					LOG_DBG("Failed to unsubscribe to state: %d", err);

					return err;
				}
			}
		}

		bt_conn_unref(conn);
		inst->conn = NULL;
	}

	/* The subscribe parameters must remain instact so they can get cleaned up by GATT */
	memset(&inst->disc_params, 0, sizeof(inst->disc_params));
	memset(&inst->recv_state_disc_params, 0, sizeof(inst->recv_state_disc_params));
	memset(&inst->read_params, 0, sizeof(inst->read_params));
	memset(&inst->write_params, 0, sizeof(inst->write_params));

	return 0;
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	struct bap_broadcast_assistant_instance *inst = inst_by_conn(conn);

	if (inst) {
		(void)broadcast_assistant_reset(inst);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected_cb,
};

int bt_bap_broadcast_assistant_discover(struct bt_conn *conn)
{
	int err;
	struct bap_broadcast_assistant_instance *inst;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	inst = &broadcast_assistants[bt_conn_index(conn)];

	/* Do not allow new discoveries while we are reading or writing */
	if (atomic_test_and_set_bit(inst->flags, BAP_BA_FLAG_BUSY)) {
		LOG_DBG("Instance is busy");
		return -EBUSY;
	}

	err = broadcast_assistant_reset(inst);
	if (err != 0) {
		atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);

		LOG_DBG("Failed to reset broadcast assistant: %d", err);

		return -ENOEXEC;
	}

	inst->conn = bt_conn_ref(conn);

	/* Discover BASS on peer, setup handles and notify */
	discover_init(inst);

	(void)memcpy(&uuid, BT_UUID_BASS, sizeof(uuid));
	inst->disc_params.func = service_discover_func;
	inst->disc_params.uuid = &uuid.uuid;
	inst->disc_params.type = BT_GATT_DISCOVER_PRIMARY;
	inst->disc_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	inst->disc_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	err = bt_gatt_discover(conn, &inst->disc_params);
	if (err != 0) {
		atomic_clear_bit(inst->flags, BAP_BA_FLAG_DISCOVER_IN_PROGRESS);
		atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);

		/* Report expected possible errors */
		if (err == -ENOTCONN || err == -ENOMEM) {
			return err;
		}

		LOG_DBG("Unexpected err %d from bt_gatt_discover", err);

		return -ENOEXEC;
	}

	return 0;
}

/* TODO: naming is different from e.g. bt_vcp_vol_ctrl_cb_register */
int bt_bap_broadcast_assistant_register_cb(struct bt_bap_broadcast_assistant_cb *cb)
{
	struct bt_bap_broadcast_assistant_cb *tmp;

	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&broadcast_assistant_cbs, tmp, _node) {
		if (tmp == cb) {
			LOG_DBG("Already registered");
			return -EALREADY;
		}
	}

	sys_slist_append(&broadcast_assistant_cbs, &cb->_node);

	return 0;
}

int bt_bap_broadcast_assistant_unregister_cb(struct bt_bap_broadcast_assistant_cb *cb)
{
	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&broadcast_assistant_cbs, &cb->_node)) {
		return -EALREADY;
	}

	return 0;
}

int bt_bap_broadcast_assistant_scan_start(struct bt_conn *conn, bool start_scan)
{
	struct bt_bap_bass_cp_scan_start *cp;
	int err;
	struct bap_broadcast_assistant_instance *inst;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	inst = inst_by_conn(conn);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (atomic_test_and_set_bit(inst->flags, BAP_BA_FLAG_BUSY)) {
		/* Do not allow writes while we are discovering as the handles may change */

		LOG_DBG("instance busy");

		return -EBUSY;
	}

	/* TODO: Remove the start_scan parameter and support from the assistant */
	if (start_scan) {
		static bool cb_registered;

		if (atomic_test_and_set_bit(inst->flags, BAP_BA_FLAG_SCANNING)) {
			LOG_DBG("Already scanning");

			atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);

			return -EALREADY;
		}

		if (!cb_registered) {
			bt_le_scan_cb_register(&scan_cb);
			cb_registered = true;
		}

		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
		if (err != 0) {
			LOG_DBG("Could not start scan (%d)", err);

			atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);
			atomic_clear_bit(inst->flags, BAP_BA_FLAG_SCANNING);

			/* Report expected possible errors */
			if (err == -EAGAIN) {
				return err;
			}

			LOG_DBG("Unexpected err %d from bt_le_scan_start", err);

			return -ENOEXEC;
		}
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&att_buf);
	cp = net_buf_simple_add(&att_buf, sizeof(*cp));

	cp->opcode = BT_BAP_BASS_OP_SCAN_START;

	err = bt_bap_broadcast_assistant_common_cp(conn, &att_buf);
	if (err != 0 && start_scan) {
		/* bt_bap_broadcast_assistant_common_cp clears the busy flag on error */
		err = bt_le_scan_stop();
		if (err != 0) {
			LOG_DBG("Could not stop scan (%d)", err);

			return -ENOEXEC;
		}

		atomic_clear_bit(inst->flags, BAP_BA_FLAG_SCANNING);
	}

	return err;
}

int bt_bap_broadcast_assistant_scan_stop(struct bt_conn *conn)
{
	struct bt_bap_bass_cp_scan_stop *cp;
	int err;
	struct bap_broadcast_assistant_instance *inst;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	inst = inst_by_conn(conn);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (atomic_test_and_set_bit(inst->flags, BAP_BA_FLAG_BUSY)) {
		LOG_DBG("instance busy");

		return -EBUSY;
	}

	if (atomic_test_bit(inst->flags, BAP_BA_FLAG_SCANNING)) {
		err = bt_le_scan_stop();
		if (err != 0) {
			LOG_DBG("Could not stop scan (%d)", err);

			atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);

			return err;
		}

		atomic_clear_bit(inst->flags, BAP_BA_FLAG_SCANNING);
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&att_buf);
	cp = net_buf_simple_add(&att_buf, sizeof(*cp));

	cp->opcode = BT_BAP_BASS_OP_SCAN_STOP;

	return bt_bap_broadcast_assistant_common_cp(conn, &att_buf);
}

static bool bis_syncs_unique_or_no_pref(uint32_t requested_bis_syncs, uint32_t aggregated_bis_syncs)
{
	if (requested_bis_syncs == 0U || aggregated_bis_syncs == 0U) {
		return true;
	}

	if (requested_bis_syncs == BT_BAP_BIS_SYNC_NO_PREF &&
	    aggregated_bis_syncs == BT_BAP_BIS_SYNC_NO_PREF) {
		return true;
	}

	return (requested_bis_syncs & aggregated_bis_syncs) != 0U;
}

static bool valid_bis_sync_request(uint32_t requested_bis_syncs, uint32_t aggregated_bis_syncs)
{
	/* Verify that the request BIS sync indexes are unique or no preference */
	if (!bis_syncs_unique_or_no_pref(requested_bis_syncs, aggregated_bis_syncs)) {
		LOG_DBG("Duplicate BIS index 0x%08x (aggregated %x)", requested_bis_syncs,
			aggregated_bis_syncs);
		return false;
	}

	if (requested_bis_syncs != BT_BAP_BIS_SYNC_NO_PREF &&
	    aggregated_bis_syncs == BT_BAP_BIS_SYNC_NO_PREF) {
		LOG_DBG("Invalid BIS index 0x%08X mixing BT_BAP_BIS_SYNC_NO_PREF and specific BIS",
			requested_bis_syncs);
		return false;
	}

	if (!valid_bis_syncs(requested_bis_syncs)) {
		LOG_DBG("Invalid BIS sync: 0x%08X", requested_bis_syncs);
		return false;
	}

	return true;
}

static bool valid_subgroup_params(uint8_t pa_sync, const struct bt_bap_bass_subgroup subgroups[],
				  uint8_t num_subgroups)
{
	uint32_t aggregated_bis_syncs = 0U;

	for (uint8_t i = 0U; i < num_subgroups; i++) {
		/* BIS sync values of 0 and BT_BAP_BIS_SYNC_NO_PREF are allowed at any time, but any
		 * other values are only allowed if PA sync state is also set
		 */
		CHECKIF(pa_sync == 0 && (subgroups[i].bis_sync != 0U &&
					 subgroups[i].bis_sync != BT_BAP_BIS_SYNC_NO_PREF)) {
			LOG_DBG("[%u]: Only syncing to BIS is not allowed", i);

			return false;
		}

		/* Verify that the request BIS sync indexes are unique or no preference */
		if (!valid_bis_sync_request(subgroups[i].bis_sync, aggregated_bis_syncs)) {
			LOG_DBG("Invalid BIS Sync request[%d]", i);

			return false;
		}

		/* Keep track of BIS sync values to ensure that we do not have duplicates */
		aggregated_bis_syncs |= subgroups[i].bis_sync;

#if defined(CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE)
		if (subgroups[i].metadata_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
			LOG_DBG("[%u]: Invalid metadata_len: %u", i, subgroups[i].metadata_len);

			return false;
		}
	}
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE */

	return true;
}

static bool valid_add_src_param(const struct bt_bap_broadcast_assistant_add_src_param *param)
{
	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return false;
	}

	CHECKIF(param->addr.type > BT_ADDR_LE_RANDOM) {
		LOG_DBG("Invalid address type %u", param->addr.type);
		return false;
	}

	CHECKIF(param->adv_sid > BT_GAP_SID_MAX) {
		LOG_DBG("Invalid adv_sid %u", param->adv_sid);
		return false;
	}

	CHECKIF(!(param->pa_interval != BT_BAP_PA_INTERVAL_UNKNOWN) &&
		!IN_RANGE(param->pa_interval, BT_GAP_PER_ADV_MIN_INTERVAL,
			  BT_GAP_PER_ADV_MAX_INTERVAL)) {
		LOG_DBG("Invalid pa_interval 0x%04X", param->pa_interval);
		return false;
	}

	CHECKIF(param->broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		LOG_DBG("Invalid broadcast_id 0x%08X", param->broadcast_id);
		return false;
	}

	CHECKIF(param->num_subgroups != 0 && param->subgroups == NULL) {
		LOG_DBG("Subgroups are NULL when num_subgroups = %u", param->num_subgroups);
		return false;
	}

	CHECKIF(param->num_subgroups > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		LOG_DBG("Too many subgroups %u/%u", param->num_subgroups,
			CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);

		return false;
	}

	CHECKIF(param->subgroups != NULL) {
		if (!valid_subgroup_params(param->pa_sync, param->subgroups,
					   param->num_subgroups)) {
			return false;
		}
	}

	return true;
}

int bt_bap_broadcast_assistant_add_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_add_src_param *param)
{
	struct bt_bap_bass_cp_add_src *cp;
	struct bap_broadcast_assistant_instance *inst;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	inst = inst_by_conn(conn);

	if (inst == NULL) {
		return -EINVAL;
	}

	/* Check if this operation would result in a duplicate before proceeding */
	if (broadcast_src_is_duplicate(inst, param->broadcast_id, param->adv_sid,
				       param->addr.type)) {
		LOG_DBG("Broadcast source already exists");

		return -EINVAL;
	}

	if (!valid_add_src_param(param)) {
		return -EINVAL;
	}

	if (inst->cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (atomic_test_and_set_bit(inst->flags, BAP_BA_FLAG_BUSY)) {
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

			/* TODO: Validate parameters before setting the busy flag to reduce cleanup
			 */
			atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);

			return -EINVAL;
		}

		subgroup = net_buf_simple_add(&att_buf, subgroup_size);

		subgroup->bis_sync = param->subgroups[i].bis_sync;

#if defined(CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE)
		if (param->subgroups[i].metadata_len != 0) {
			(void)memcpy(subgroup->metadata, param->subgroups[i].metadata,
				     param->subgroups[i].metadata_len);
			subgroup->metadata_len = param->subgroups[i].metadata_len;
		} else {
			subgroup->metadata_len = 0U;
		}
#else
		subgroup->metadata_len = 0U;
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE */
	}

	return bt_bap_broadcast_assistant_common_cp(conn, &att_buf);
}

static bool valid_add_mod_param(const struct bt_bap_broadcast_assistant_mod_src_param *param)
{
	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return false;
	}

	CHECKIF(!(param->pa_interval != BT_BAP_PA_INTERVAL_UNKNOWN) &&
		!IN_RANGE(param->pa_interval, BT_GAP_PER_ADV_MIN_INTERVAL,
			  BT_GAP_PER_ADV_MAX_INTERVAL)) {
		LOG_DBG("Invalid pa_interval 0x%04X", param->pa_interval);
		return false;
	}

	CHECKIF(param->num_subgroups != 0 && param->subgroups == NULL) {
		LOG_DBG("Subgroups are NULL when num_subgroups = %u", param->num_subgroups);
		return false;
	}

	CHECKIF(param->num_subgroups > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		LOG_DBG("Too many subgroups %u/%u", param->num_subgroups,
			CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);

		return false;
	}

	CHECKIF(param->subgroups != NULL) {
		if (!valid_subgroup_params(param->pa_sync, param->subgroups,
					   param->num_subgroups)) {
			return false;
		}
	}

	return true;
}

int bt_bap_broadcast_assistant_mod_src(struct bt_conn *conn,
				       const struct bt_bap_broadcast_assistant_mod_src_param *param)
{
	struct bt_bap_bass_cp_mod_src *cp;
	bool known_recv_state;
	bool past_avail;
	struct bap_broadcast_assistant_instance *inst;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	if (!valid_add_mod_param(param)) {
		return -EINVAL;
	}

	inst = inst_by_conn(conn);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (atomic_test_and_set_bit(inst->flags, BAP_BA_FLAG_BUSY)) {
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
	for (size_t i = 0; i < ARRAY_SIZE(inst->recv_states); i++) {
		if (inst->recv_states[i].src_id == param->src_id) {
			known_recv_state = true;
			past_avail = inst->recv_states[i].past_avail;
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

			/* TODO: Validate parameters before setting the busy flag to reduce cleanup
			 */
			atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);

			return -EINVAL;
		}
		subgroup = net_buf_simple_add(&att_buf, subgroup_size);

		subgroup->bis_sync = param->subgroups[i].bis_sync;

#if defined(CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE)
		if (param->subgroups[i].metadata_len != 0) {
			(void)memcpy(subgroup->metadata,
				     param->subgroups[i].metadata,
				     param->subgroups[i].metadata_len);
			subgroup->metadata_len = param->subgroups[i].metadata_len;
		} else {
			subgroup->metadata_len = 0U;
		}
#else
		subgroup->metadata_len = 0U;
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE */
	}

	return bt_bap_broadcast_assistant_common_cp(conn, &att_buf);
}

int bt_bap_broadcast_assistant_set_broadcast_code(
	struct bt_conn *conn, uint8_t src_id,
	const uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE])
{
	struct bt_bap_bass_cp_broadcase_code *cp;
	struct bap_broadcast_assistant_instance *inst;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	inst = inst_by_conn(conn);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (atomic_test_and_set_bit(inst->flags, BAP_BA_FLAG_BUSY)) {
		LOG_DBG("instance busy");

		return -EBUSY;
	}

	/* Reset buffer before using */
	net_buf_simple_reset(&att_buf);
	cp = net_buf_simple_add(&att_buf, sizeof(*cp));

	cp->opcode = BT_BAP_BASS_OP_BROADCAST_CODE;
	cp->src_id = src_id;

	(void)memcpy(cp->broadcast_code, broadcast_code, BT_ISO_BROADCAST_CODE_SIZE);

	LOG_HEXDUMP_DBG(cp->broadcast_code, BT_ISO_BROADCAST_CODE_SIZE, "broadcast code:");

	return bt_bap_broadcast_assistant_common_cp(conn, &att_buf);
}

int bt_bap_broadcast_assistant_rem_src(struct bt_conn *conn, uint8_t src_id)
{
	struct bt_bap_bass_cp_rem_src *cp;
	struct bap_broadcast_assistant_instance *inst;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	inst = inst_by_conn(conn);
	if (inst == NULL) {
		return -EINVAL;
	}

	if (inst->cp_handle == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (atomic_test_and_set_bit(inst->flags, BAP_BA_FLAG_BUSY)) {
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

static int read_recv_state(struct bap_broadcast_assistant_instance *inst, uint8_t idx)
{
	int err;

	inst->read_params.func = read_recv_state_cb;
	inst->read_params.handle_count = 1;
	inst->read_params.single.handle = inst->recv_state_handles[idx];

	err = bt_gatt_read(inst->conn, &inst->read_params);
	if (err != 0) {
		(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	}

	return err;
}

int bt_bap_broadcast_assistant_read_recv_state(struct bt_conn *conn,
					       uint8_t idx)
{
	int err;
	struct bap_broadcast_assistant_instance *inst;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");

		return -EINVAL;
	}

	inst = inst_by_conn(conn);
	if (inst == NULL) {
		return -EINVAL;
	}

	CHECKIF(idx >= ARRAY_SIZE(inst->recv_state_handles)) {
		LOG_DBG("Invalid idx: %u", idx);

		return -EINVAL;
	}

	if (inst->recv_state_handles[idx] == 0) {
		LOG_DBG("handle not set");

		return -EINVAL;
	} else if (atomic_test_and_set_bit(inst->flags, BAP_BA_FLAG_BUSY)) {
		LOG_DBG("instance busy");

		return -EBUSY;
	}

	err = read_recv_state(inst, idx);
	if (err != 0) {
		(void)memset(&inst->read_params, 0,
			     sizeof(inst->read_params));
		atomic_clear_bit(inst->flags, BAP_BA_FLAG_BUSY);
	}

	return err;
}
