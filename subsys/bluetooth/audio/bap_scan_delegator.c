/*  Bluetooth Scan Delegator */

/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(bt_bap_scan_delegator, CONFIG_BT_BAP_SCAN_DELEGATOR_LOG_LEVEL);

#include "common/bt_str.h"

#include "audio_internal.h"
#include "bap_internal.h"
#include "../host/conn_internal.h"
#include "../host/hci_core.h"

#define PAST_TIMEOUT              K_SECONDS(10)

#define SCAN_DELEGATOR_BUF_SEM_TIMEOUT K_MSEC(CONFIG_BT_BAP_SCAN_DELEGATOR_BUF_TIMEOUT)
NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

struct bass_recv_state_flags {
	bool updated: 1;
};

/* TODO: Merge bass_recv_state_internal_t and bt_bap_scan_delegator_recv_state */
struct bass_recv_state_internal {
	const struct bt_gatt_attr *attr;

	bool active;
	uint8_t index;
	struct bt_bap_scan_delegator_recv_state state;
	uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE];

	/** Requested BIS sync bitfield for each subgroup */
	uint32_t requested_bis_sync[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS];

	/* Mutex (reentrant Locking) ensure multiple threads to safely access receive state data */
	struct k_mutex mutex;
	struct bass_recv_state_flags flags[CONFIG_BT_MAX_CONN];

	struct k_work_delayable notify_work;
};

struct bt_bap_scan_delegator_inst {
	uint8_t next_src_id;
	struct bass_recv_state_internal recv_states
		[CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT];
};

enum scan_delegator_flag {
	SCAN_DELEGATOR_FLAG_REGISTERED_CONN_CB,
	SCAN_DELEGATOR_FLAG_REGISTERED_SCAN_DELIGATOR,

	SCAN_DELEGATOR_FLAG_NUM,
};

static ATOMIC_DEFINE(scan_delegator_flags, SCAN_DELEGATOR_FLAG_NUM);

static struct bt_bap_scan_delegator_inst scan_delegator;
static struct bt_bap_scan_delegator_cb *scan_delegator_cbs;

static void set_receive_state_changed_cb(struct bt_conn *conn, void *data)
{
	struct bass_recv_state_internal *internal_state = data;
	struct bass_recv_state_flags *flags = &internal_state->flags[bt_conn_index(conn)];
	struct bt_conn_info conn_info;
	int err;

	err = bt_conn_get_info(conn, &conn_info);
	__ASSERT_NO_MSG(err == 0);

	if (conn_info.state != BT_CONN_STATE_CONNECTED ||
	    !bt_gatt_is_subscribed(conn, internal_state->attr, BT_GATT_CCC_NOTIFY)) {
		return;
	}

	flags->updated = true;

	/* We may schedule the same work multiple times, but that is OK as scheduling the same work
	 * multiple times is a no-op
	 */
	err = k_work_schedule(&internal_state->notify_work, K_NO_WAIT);
	__ASSERT(err >= 0, "Failed to schedule work: %d", err);
}

static void set_receive_state_changed(struct bass_recv_state_internal *internal_state)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, set_receive_state_changed_cb, (void *)internal_state);
}

/**
 * @brief Returns whether a value's bits is a subset of another value's bits
 *
 * @param a The subset to check
 * @param b The bits to check against
 *
 * @return True if @p a is a bitwise subset of @p b
 */
static bool bits_subset_of(uint32_t a, uint32_t b)
{
	return (((a) & (~(b))) == 0);
}

static bool bis_syncs_unique_or_no_pref(uint32_t requested_bis_syncs,
					uint32_t aggregated_bis_syncs)
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

static void bt_debug_dump_recv_state(const struct bass_recv_state_internal *recv_state)
{
	if (recv_state->active) {
		const struct bt_bap_scan_delegator_recv_state *state = &recv_state->state;
		const bool is_bad_code = state->encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE;

		LOG_DBG("Receive State[%d]: src ID %u, addr %s, adv_sid %u, broadcast_id 0x%06X, "
			"pa_sync_state %u, encrypt state %u%s%s, num_subgroups %u",
			recv_state->index, state->src_id, bt_addr_le_str(&state->addr),
			state->adv_sid, state->broadcast_id, state->pa_sync_state,
			state->encrypt_state, is_bad_code ? ", bad code" : "",
			is_bad_code ? bt_hex(state->bad_code, sizeof(state->bad_code)) : "",
			state->num_subgroups);

		for (uint8_t i = 0U; i < state->num_subgroups; i++) {
			const struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];

			LOG_DBG("\tSubgroup[%u]: BIS sync %u (requested %u), metadata_len %zu, "
				"metadata: %s",
				i, subgroup->bis_sync, recv_state->requested_bis_sync[i],
				subgroup->metadata_len,
				bt_hex(subgroup->metadata, subgroup->metadata_len));
		}
	} else {
		LOG_DBG("Inactive receive state");
	}
}

static void receive_state_notify_cb(struct bt_conn *conn, void *data)
{
	struct bass_recv_state_internal *internal_state = data;
	struct bass_recv_state_flags *flags = &internal_state->flags[bt_conn_index(conn)];
	struct bt_conn_info conn_info;
	int err;

	err = bt_conn_get_info(conn, &conn_info);
	__ASSERT_NO_MSG(err == 0);

	if (conn_info.state != BT_CONN_STATE_CONNECTED ||
	    !bt_gatt_is_subscribed(conn, internal_state->attr, BT_GATT_CCC_NOTIFY)) {
		return;
	}

	if (flags->updated) {
		uint16_t max_ntf_size;
		uint16_t ntf_size;

		max_ntf_size = bt_audio_get_max_ntf_size(conn);

		ntf_size = MIN(max_ntf_size, read_buf.len);
		if (ntf_size < read_buf.len) {
			LOG_DBG("Sending truncated notification (%u/%u)", ntf_size, read_buf.len);
		}

		LOG_DBG("Sending bytes %u for %p", ntf_size, (void *)conn);
		err = bt_gatt_notify_uuid(conn, BT_UUID_BASS_RECV_STATE, internal_state->attr,
					  read_buf.data, ntf_size);
		if (err == 0) {
			flags->updated = false;
			return;
		}

		LOG_DBG("Could not notify receive state: %d", err);
		err = k_work_reschedule(&internal_state->notify_work,
					K_USEC(BT_CONN_INTERVAL_TO_US(conn_info.le.interval)));
		__ASSERT(err >= 0, "Failed to reschedule work: %d", err);
	}
}

static void net_buf_put_recv_state(const struct bass_recv_state_internal *recv_state)
{
	const struct bt_bap_scan_delegator_recv_state *state = &recv_state->state;

	net_buf_simple_reset(&read_buf);

	__ASSERT(recv_state, "NULL receive state");

	if (!recv_state->active) {
		/* Notify empty */

		return;
	}

	(void)net_buf_simple_add_u8(&read_buf, state->src_id);
	(void)net_buf_simple_add_u8(&read_buf, state->addr.type);
	(void)net_buf_simple_add_mem(&read_buf, &state->addr.a,
				     sizeof(state->addr.a));
	(void)net_buf_simple_add_u8(&read_buf, state->adv_sid);
	(void)net_buf_simple_add_le24(&read_buf, state->broadcast_id);
	(void)net_buf_simple_add_u8(&read_buf, state->pa_sync_state);
	(void)net_buf_simple_add_u8(&read_buf, state->encrypt_state);
	if (state->encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE) {
		(void)net_buf_simple_add_mem(&read_buf, &state->bad_code,
					     sizeof(state->bad_code));
	}
	(void)net_buf_simple_add_u8(&read_buf, state->num_subgroups);
	for (int i = 0; i < state->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];

		(void)net_buf_simple_add_le32(&read_buf, subgroup->bis_sync);
		(void)net_buf_simple_add_u8(&read_buf, subgroup->metadata_len);
		(void)net_buf_simple_add_mem(&read_buf, subgroup->metadata,
					     subgroup->metadata_len);
	}
}

static void receive_state_updated(struct bt_conn *conn,
				  struct bass_recv_state_internal *internal_state)
{
	if (IS_ENABLED(CONFIG_BT_BAP_SCAN_DELEGATOR_LOG_LEVEL_DBG)) {
		int err;

		err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
		if (err == 0) {
			bt_debug_dump_recv_state(internal_state);
			err = k_mutex_unlock(&internal_state->mutex);
			__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

		} else {
			LOG_DBG("Failed to lock mutex: %d", err);
		}
	}

	if (scan_delegator_cbs != NULL && scan_delegator_cbs->recv_state_updated != NULL) {
		scan_delegator_cbs->recv_state_updated(conn, &internal_state->state);
	}
}

static void notify_work_handler(struct k_work *work)
{
	struct bass_recv_state_internal *internal_state = CONTAINER_OF(
		k_work_delayable_from_work(work), struct bass_recv_state_internal, notify_work);
	int err;

	err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to take mutex: %d", err);
		err = k_work_reschedule(&internal_state->notify_work, K_NO_WAIT);
		__ASSERT(err >= 0, "Failed to reschedule work: %d", err);
		return;
	}

	net_buf_put_recv_state(internal_state);
	bt_conn_foreach(BT_CONN_TYPE_LE, receive_state_notify_cb, internal_state);

	err = k_mutex_unlock(&internal_state->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
}

static void bis_sync_request_updated(struct bt_conn *conn,
				     const struct bass_recv_state_internal *internal_state)
{
	if (scan_delegator_cbs != NULL &&
	    scan_delegator_cbs->bis_sync_req != NULL) {
		scan_delegator_cbs->bis_sync_req(conn, &internal_state->state,
						 internal_state->requested_bis_sync);
	} else {
		LOG_WRN("bis_sync_req callback is missing");
	}
}

static void scan_delegator_security_changed(struct bt_conn *conn,
					    bt_security_t level,
					    enum bt_security_err err)
{

	if (err != 0 || level < BT_SECURITY_L2 || !bt_le_bond_exists(conn->id, &conn->le.dst)) {
		return;
	}

	/* Notify all receive states after a bonded device reconnects */
	for (size_t i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		const struct bass_recv_state_internal *internal_state =
			&scan_delegator.recv_states[i];

		if (!internal_state->active) {
			continue;
		}

		set_receive_state_changed_cb(conn, (void *)internal_state);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.security_changed = scan_delegator_security_changed,
};

static uint8_t next_src_id(void)
{
	uint8_t next_src_id;
	bool unique = false;

	while (!unique) {
		next_src_id = scan_delegator.next_src_id++;
		unique = true;
		for (size_t i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
			if (scan_delegator.recv_states[i].active &&
			    scan_delegator.recv_states[i].state.src_id == next_src_id) {
				unique = false;
				break;
			}
		}
	}

	return next_src_id;
}

static struct bass_recv_state_internal *bass_lookup_src_id(uint8_t src_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		if (scan_delegator.recv_states[i].active &&
		    scan_delegator.recv_states[i].state.src_id == src_id) {
			return &scan_delegator.recv_states[i];
		}
	}

	return NULL;
}

/* BAP 6.5.4 states that the combined Source_Address_Type, Source_Adv_SID, and Broadcast_ID fields
 * are what makes a receive state unique.
 */
static struct bass_recv_state_internal *bass_lookup_state(uint8_t addr_type, uint8_t adv_sid,
							  uint32_t broadcast_id)
{
	struct bass_recv_state_internal *res = NULL;

	ARRAY_FOR_EACH_PTR(scan_delegator.recv_states, recv_state_internal) {
		int err;

		if (!recv_state_internal->active) {
			continue;
		}

		err = k_mutex_lock(&recv_state_internal->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
		if (err != 0) {
			LOG_DBG("Failed to lock mutex: %d", err);

			return NULL;
		}

		if (recv_state_internal->state.addr.type == addr_type &&
		    recv_state_internal->state.adv_sid == adv_sid &&
		    recv_state_internal->state.broadcast_id == broadcast_id) {
			res = recv_state_internal;
		}

		err = k_mutex_unlock(&recv_state_internal->mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

		if (res != NULL) {
			break;
		}
	}

	return res;
}

static struct bass_recv_state_internal *get_free_recv_state(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		struct bass_recv_state_internal *free_internal_state =
			&scan_delegator.recv_states[i];

		if (!free_internal_state->active) {
			return free_internal_state;
		}
	}

	return NULL;
}

static bool supports_past(struct bt_conn *conn, uint8_t pa_sync_val)
{
	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER)) {
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

		LOG_DBG("%p remote %s PAST, local %s PAST (req %u)", (void *)conn,
			BT_FEAT_LE_PAST_SEND(remote_info.le.features) ? "supports"
								      : "does not support",
			BT_FEAT_LE_PAST_RECV(local_features.features) ? "supports"
								      : "does not support",
			pa_sync_val);

		return pa_sync_val == BT_BAP_BASS_PA_REQ_SYNC_PAST &&
		       BT_FEAT_LE_PAST_SEND(remote_info.le.features) &&
		       BT_FEAT_LE_PAST_RECV(local_features.features);
	} else {
		return false;
	}
}

static int pa_sync_request(struct bt_conn *conn,
			   const struct bt_bap_scan_delegator_recv_state *state,
			   uint8_t pa_sync_val, uint16_t pa_interval)
{
	int err = -EACCES;

	if (scan_delegator_cbs != NULL &&
	    scan_delegator_cbs->pa_sync_req != NULL) {
		const bool past_supported = supports_past(conn, pa_sync_val);

		err = scan_delegator_cbs->pa_sync_req(conn, state,
						      past_supported,
						      pa_interval);
	} else {
		LOG_WRN("pa_sync_req callback is missing, rejecting PA sync request");
	}

	return err;
}

static int pa_sync_term_request(struct bt_conn *conn,
				const struct bt_bap_scan_delegator_recv_state *state)
{
	int err = -EACCES;

	if (scan_delegator_cbs != NULL &&
	    scan_delegator_cbs->pa_sync_req != NULL) {
		err = scan_delegator_cbs->pa_sync_term_req(conn, state);
	} else {
		LOG_WRN("pa_sync_term_req callback is missing, rejecting PA sync term request");
	}

	return err;
}

static int scan_delegator_add_src(struct bt_conn *conn,
				     struct net_buf_simple *buf)
{
	struct bass_recv_state_internal *internal_state = NULL;
	struct bt_bap_scan_delegator_recv_state *state;
	bt_addr_le_t *addr;
	uint8_t pa_sync;
	uint16_t pa_interval;
	uint32_t aggregated_bis_syncs = 0;
	uint32_t broadcast_id;
	bool bis_sync_requested;
	uint16_t total_len;
	struct bt_bap_bass_cp_add_src *add_src;
	int ret = BT_GATT_ERR(BT_ATT_ERR_SUCCESS);
	uint8_t adv_sid;
	int err;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len < sizeof(struct bt_bap_bass_cp_add_src) - 1) {
		LOG_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	add_src = (void *)(buf->data - 1);
	total_len = sizeof(struct bt_bap_bass_cp_add_src) - 1;
	for (int i = 0; i < add_src->num_subgroups; i++) {
		struct bt_bap_bass_cp_subgroup *subgroup;
		uint16_t index = total_len;

		total_len += sizeof(struct bt_bap_bass_cp_subgroup);
		if (total_len > buf->len) {
			LOG_DBG("Invalid length %u", buf->len);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}

		subgroup = (void *)&buf->data[index];
		total_len += subgroup->metadata_len;
		if (total_len > buf->len) {
			LOG_DBG("Invalid length %u", buf->len);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}
	}

	if (total_len != buf->len) {
		LOG_DBG("Invalid length %u", buf->len);

		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	addr = net_buf_simple_pull_mem(buf, sizeof(*addr));
	if (addr->type > BT_ADDR_LE_RANDOM) {
		LOG_DBG("Invalid address type %u", addr->type);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	adv_sid = net_buf_simple_pull_u8(buf);
	if (adv_sid > BT_GAP_SID_MAX) {
		LOG_DBG("Invalid adv SID %u", adv_sid);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	broadcast_id = net_buf_simple_pull_le24(buf);

	internal_state = bass_lookup_state(addr->type, adv_sid, broadcast_id);
	if (internal_state != NULL) {
		LOG_DBG("Adding addr type=0x%02X adv_sid=0x%02X and broadcast_id=0x%06X would "
			"result in duplication",
			addr->type, adv_sid, broadcast_id);

		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	internal_state = get_free_recv_state();
	if (internal_state == NULL) {
		LOG_DBG("Could not get free receive state");

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to lock mutex: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	state = &internal_state->state;

	state->src_id = next_src_id();
	bt_addr_le_copy(&state->addr, addr);
	state->adv_sid = adv_sid;
	state->broadcast_id = broadcast_id;

	pa_sync = net_buf_simple_pull_u8(buf);
	if (pa_sync > BT_BAP_BASS_PA_REQ_SYNC) {
		LOG_DBG("Invalid PA sync value %u", pa_sync);
		ret = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		goto unlock_return;
	}

	pa_interval = net_buf_simple_pull_le16(buf);

	state->num_subgroups = net_buf_simple_pull_u8(buf);
	if (state->num_subgroups > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		LOG_WRN("Too many subgroups %u/%u", state->num_subgroups,
			CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);
		ret = BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		goto unlock_return;
	}

	bis_sync_requested = false;
	for (int i = 0; i < state->num_subgroups; i++) {
		struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];
		uint8_t *metadata;

		internal_state->requested_bis_sync[i] = net_buf_simple_pull_le32(buf);

		if (internal_state->requested_bis_sync[i] &&
		    pa_sync == BT_BAP_BASS_PA_REQ_NO_SYNC) {
			LOG_DBG("Cannot sync to BIS without PA");
			ret = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
			goto unlock_return;
		}

		if (internal_state->requested_bis_sync[i] != 0U) {
			bis_sync_requested = true;
		}

		if (!valid_bis_sync_request(internal_state->requested_bis_sync[i],
					    aggregated_bis_syncs)) {
			LOG_DBG("Invalid BIS Sync request[%d]", i);
			ret = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
			goto unlock_return;
		}

		aggregated_bis_syncs |= internal_state->requested_bis_sync[i];

		subgroup->metadata_len = net_buf_simple_pull_u8(buf);

		if (subgroup->metadata_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
			LOG_WRN("Metadata too long %u/%u", subgroup->metadata_len,
				CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE);
			ret = BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
			goto unlock_return;
		}

		metadata = net_buf_simple_pull_mem(buf, subgroup->metadata_len);
		(void)memcpy(subgroup->metadata, metadata,
			     subgroup->metadata_len);
	}

	if (scan_delegator_cbs != NULL && scan_delegator_cbs->add_source != NULL) {
		err = scan_delegator_cbs->add_source(conn, state);
		if (err != 0) {
			LOG_DBG("add_source callback rejected: 0x%02x", err);
			ret = BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
			goto unlock_return;
		}
	}

	/* The active flag shall be set before any application callbacks, so that any calls for the
	 * receive state can be processed
	 */
	internal_state->active = true;

	if (pa_sync != BT_BAP_BASS_PA_REQ_NO_SYNC) {
		/* Unlock mutex to avoid potential deadlock on app callback */
		err = k_mutex_unlock(&internal_state->mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

		err = pa_sync_request(conn, state, pa_sync, pa_interval);
		if (err != 0) {
			err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
			if (err != 0) {
				LOG_DBG("Failed to lock mutex: %d", err);

				return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
			}

			(void)memset(state, 0, sizeof(*state));
			internal_state->active = false;

			LOG_DBG("PA sync %u from %p was rejected with reason %d", pa_sync,
				(void *)conn, err);

			err = k_mutex_unlock(&internal_state->mutex);
			__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}

		err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
		if (err != 0) {
			LOG_DBG("Failed to lock mutex: %d", err);

			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}
	}

	LOG_DBG("Index %u: New source added: ID 0x%02x",
		internal_state->index, state->src_id);

	set_receive_state_changed(internal_state);

unlock_return:
	err = k_mutex_unlock(&internal_state->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	if (ret == BT_GATT_ERR(BT_ATT_ERR_SUCCESS)) {
		/* app callback */
		receive_state_updated(conn, internal_state);

		if (bis_sync_requested) {
			bis_sync_request_updated(conn, internal_state);
		}
	}

	return ret;
}

static int scan_delegator_mod_src(struct bt_conn *conn,
				  struct net_buf_simple *buf)
{
	uint32_t requested_bis_sync[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS] = {};
	struct bt_bap_scan_delegator_recv_state backup_state;
	struct bass_recv_state_internal *internal_state;
	struct bt_bap_scan_delegator_recv_state *state;
	uint8_t src_id;
	bool state_changed = false;
	uint16_t pa_interval;
	uint8_t num_subgroups;
	struct bt_bap_bass_subgroup
		subgroups[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS] = { 0 };
	uint8_t pa_sync;
	uint32_t aggregated_bis_syncs = 0;
	bool bis_sync_change_requested;
	uint16_t total_len;
	struct bt_bap_bass_cp_mod_src *mod_src;
	int ret = BT_GATT_ERR(BT_ATT_ERR_SUCCESS);
	int err;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len < sizeof(struct bt_bap_bass_cp_mod_src) - 1) {
		LOG_DBG("Invalid length %u", buf->len);

		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	mod_src = (void *)(buf->data - 1);
	total_len = sizeof(struct bt_bap_bass_cp_mod_src) - 1;
	for (int i = 0; i < mod_src->num_subgroups; i++) {
		struct bt_bap_bass_cp_subgroup *subgroup;
		uint16_t index = total_len;

		total_len += sizeof(struct bt_bap_bass_cp_subgroup);
		if (total_len > buf->len) {
			LOG_DBG("Invalid length %u", buf->len);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}

		subgroup = (void *)&buf->data[index];
		total_len += subgroup->metadata_len;
		if (total_len > buf->len) {
			LOG_DBG("Invalid length %u", buf->len);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}
	}

	if (total_len != buf->len) {
		LOG_DBG("Invalid length %u", buf->len);

		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	src_id = net_buf_simple_pull_u8(buf);
	internal_state = bass_lookup_src_id(src_id);

	LOG_DBG("src_id %u", src_id);

	if (internal_state == NULL) {
		LOG_DBG("Could not find state by src id %u", src_id);

		return BT_GATT_ERR(BT_BAP_BASS_ERR_INVALID_SRC_ID);
	}

	pa_sync = net_buf_simple_pull_u8(buf);
	if (pa_sync > BT_BAP_BASS_PA_REQ_SYNC) {
		LOG_DBG("Invalid PA sync value %u", pa_sync);

		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	pa_interval = net_buf_simple_pull_le16(buf);

	num_subgroups = net_buf_simple_pull_u8(buf);
	if (num_subgroups > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		LOG_WRN("Too many subgroups %u/%u", num_subgroups,
			CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to lock mutex: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	bis_sync_change_requested = false;
	for (int i = 0; i < num_subgroups; i++) {
		struct bt_bap_bass_subgroup *subgroup = &subgroups[i];
		uint8_t *metadata;

		requested_bis_sync[i] = net_buf_simple_pull_le32(buf);

		if (requested_bis_sync[i] != 0U && pa_sync == BT_BAP_BASS_PA_REQ_NO_SYNC) {
			LOG_DBG("Cannot sync to BIS without PA");
			ret = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
			goto unlock_return;
		}

		/* If the BIS sync request is different than what was previously was requested, or
		 * different than what we are current synced to, we set bis_sync_change_requested to
		 * let the application know that the state may need a change
		 */
		if (internal_state->requested_bis_sync[i] != requested_bis_sync[i] ||
		    internal_state->state.subgroups[i].bis_sync != requested_bis_sync[i]) {
			bis_sync_change_requested = true;
		}

		if (!valid_bis_sync_request(internal_state->requested_bis_sync[i],
					    aggregated_bis_syncs)) {
			LOG_DBG("Invalid BIS Sync request[%d]", i);
			ret = BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
			goto unlock_return;
		}
		aggregated_bis_syncs |= requested_bis_sync[i];

		subgroup->metadata_len = net_buf_simple_pull_u8(buf);

		if (subgroup->metadata_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
			LOG_WRN("Metadata too long %u/%u", subgroup->metadata_len,
				CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE);
			ret = BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
			goto unlock_return;
		}

		metadata = net_buf_simple_pull_mem(buf, subgroup->metadata_len);

		(void)memcpy(subgroup->metadata, metadata,
			     subgroup->metadata_len);
	}

	/* All input has been validated; update receive state and check for changes */
	state = &internal_state->state;

	/* Store backup in case upper layers rejects */
	(void)memcpy(&backup_state, state, sizeof(backup_state));

	if (state->num_subgroups != num_subgroups) {
		state->num_subgroups = num_subgroups;
		state_changed = true;
	}

	for (int i = 0; i < num_subgroups; i++) {
		const bool metadata_len_changed =
			subgroups[i].metadata_len != state->subgroups[i].metadata_len;

		if (metadata_len_changed) {
			state->subgroups[i].metadata_len = subgroups[i].metadata_len;
			state_changed = true;
		}

		if (metadata_len_changed ||
		    memcmp(subgroups[i].metadata, state->subgroups[i].metadata,
			   sizeof(subgroups[i].metadata)) != 0) {

			if (state->subgroups[i].metadata_len == 0U) {
				memset(state->subgroups[i].metadata, 0,
				       state->subgroups[i].metadata_len);
			} else {
				(void)memcpy(state->subgroups[i].metadata, subgroups[i].metadata,
					     state->subgroups[i].metadata_len);
			}

			state_changed = true;
		}
	}

	if (scan_delegator_cbs != NULL && scan_delegator_cbs->modify_source != NULL) {
		err = scan_delegator_cbs->modify_source(conn, state);
		if (err != 0) {
			LOG_DBG("Modify Source rejected with reason 0x%02x", err);
			(void)memcpy(state, &backup_state, sizeof(backup_state));

			err = k_mutex_unlock(&internal_state->mutex);
			__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}
	}

	/* Only send the sync request to upper layers if it is requested, and
	 * we are not already synced to the device
	 */
	if (pa_sync != BT_BAP_BASS_PA_REQ_NO_SYNC &&
	    state->pa_sync_state != BT_BAP_PA_STATE_SYNCED) {
		const uint8_t pa_sync_state = state->pa_sync_state;

		/* Unlock mutex to avoid potential deadlock on app callback */
		err = k_mutex_unlock(&internal_state->mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

		err = pa_sync_request(conn, state, pa_sync, pa_interval);
		if (err != 0) {
			err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
			if (err != 0) {
				LOG_DBG("Failed to lock mutex: %d", err);

				return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
			}

			/* Restore backup */
			(void)memcpy(state, &backup_state, sizeof(backup_state));

			LOG_DBG("PA sync %u from %p was rejected with reason %d", pa_sync,
				(void *)conn, err);

			err = k_mutex_unlock(&internal_state->mutex);
			__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		} else if (pa_sync_state != state->pa_sync_state) {
			/* Temporary work around if the state is changed when pa_sync_request is
			 * called. See https://github.com/zephyrproject-rtos/zephyr/issues/79308 for
			 * more information about this issue.
			 */
			state_changed = true;
		}

		err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
		if (err != 0) {
			LOG_DBG("Failed to lock mutex: %d", err);

			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}
	} else if (pa_sync == BT_BAP_BASS_PA_REQ_NO_SYNC &&
		   (state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ ||
		    state->pa_sync_state == BT_BAP_PA_STATE_SYNCED)) {
		/* Terminate PA sync */
		err = pa_sync_term_request(conn, &internal_state->state);

		if (err != 0) {
			LOG_DBG("PA sync term from %p was rejected with reason %d", (void *)conn,
				err);
			err = k_mutex_unlock(&internal_state->mutex);
			__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}
		state_changed = true;
	}

	/* Store requested_bis_sync after everything has been validated */
	(void)memcpy(internal_state->requested_bis_sync, requested_bis_sync,
		     sizeof(requested_bis_sync));

	/* Notify if changed */
	if (state_changed) {
		LOG_DBG("Index %u: Source modified: ID 0x%02x", internal_state->index,
			state->src_id);
		set_receive_state_changed(internal_state);
	}

unlock_return:
	err = k_mutex_unlock(&internal_state->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	if (state_changed) {
		/* app callback */
		receive_state_updated(conn, internal_state);
	}

	if (bis_sync_change_requested) {
		bis_sync_request_updated(conn, internal_state);
	}

	return ret;
}

static int scan_delegator_broadcast_code(struct bt_conn *conn,
					 struct net_buf_simple *buf)
{
	struct bass_recv_state_internal *internal_state;
	uint8_t src_id;
	const uint8_t *broadcast_code;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len != sizeof(struct bt_bap_bass_cp_broadcase_code) - 1) {
		LOG_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	src_id = net_buf_simple_pull_u8(buf);
	internal_state = bass_lookup_src_id(src_id);

	if (internal_state == NULL) {
		LOG_DBG("Could not find state by src id %u", src_id);
		return BT_GATT_ERR(BT_BAP_BASS_ERR_INVALID_SRC_ID);
	}

	broadcast_code = net_buf_simple_pull_mem(buf, sizeof(internal_state->broadcast_code));

	(void)memcpy(internal_state->broadcast_code, broadcast_code,
		     sizeof(internal_state->broadcast_code));

	LOG_DBG("Index %u: broadcast code added: %s", internal_state->index,
		bt_hex(internal_state->broadcast_code, sizeof(internal_state->broadcast_code)));

	if (scan_delegator_cbs != NULL &&
	    scan_delegator_cbs->broadcast_code != NULL) {
		scan_delegator_cbs->broadcast_code(conn, &internal_state->state,
						   broadcast_code);
	}

	return 0;
}

static int scan_delegator_rem_src(struct bt_conn *conn,
				  struct net_buf_simple *buf)
{
	struct bass_recv_state_internal *internal_state;
	struct bt_bap_scan_delegator_recv_state *state;
	bool bis_sync_was_requested = false;
	uint8_t src_id;
	int err;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len != sizeof(struct bt_bap_bass_cp_rem_src) - 1) {
		LOG_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	src_id = net_buf_simple_pull_u8(buf);
	internal_state = bass_lookup_src_id(src_id);

	if (internal_state == NULL) {
		LOG_DBG("Could not find state by src id %u", src_id);
		return BT_GATT_ERR(BT_BAP_BASS_ERR_INVALID_SRC_ID);
	}

	err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to lock mutex: %d", err);

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	state = &internal_state->state;

	/* If conn == NULL then it's a local operation and we do not need to ask the application */
	if (conn != NULL) {

		if (scan_delegator_cbs != NULL && scan_delegator_cbs->remove_source != NULL) {
			err = scan_delegator_cbs->remove_source(conn, src_id);
			if (err != 0) {
				LOG_DBG("Remove Source rejected with reason 0x%02x", err);
				err = k_mutex_unlock(&internal_state->mutex);
				__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
				return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
			}
		}

		if (state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ ||
		    state->pa_sync_state == BT_BAP_PA_STATE_SYNCED) {
			/* Terminate PA sync */
			err = pa_sync_term_request(conn, &internal_state->state);
			if (err != 0) {
				LOG_DBG("PA sync term from %p was rejected with reason %d",
					(void *)conn, err);
				err = k_mutex_unlock(&internal_state->mutex);
				__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
				return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
			}
		}
	}

	for (uint8_t i = 0U; i < state->num_subgroups; i++) {
		if (internal_state->requested_bis_sync[i] != 0U &&
		    internal_state->state.subgroups[i].bis_sync != 0U) {
			bis_sync_was_requested = true;
			break;
		}
	}

	LOG_DBG("Index %u: Removed source with ID 0x%02x",
		internal_state->index, src_id);

	internal_state->active = false;
	(void)memset(&internal_state->state, 0, sizeof(internal_state->state));
	(void)memset(internal_state->broadcast_code, 0,
		     sizeof(internal_state->broadcast_code));
	(void)memset(internal_state->requested_bis_sync, 0,
		     sizeof(internal_state->requested_bis_sync));

	set_receive_state_changed(internal_state);

	err = k_mutex_unlock(&internal_state->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	if (bis_sync_was_requested) {
		bis_sync_request_updated(conn, internal_state);
	}

	/* app callback */
	receive_state_updated(conn, internal_state);

	return BT_GATT_ERR(BT_ATT_ERR_SUCCESS);
}

static ssize_t write_control_point(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *data, uint16_t len,
				   uint16_t offset, uint8_t flags)
{
	struct net_buf_simple buf;
	uint8_t opcode;
	int err;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	} else if (len == 0) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	opcode = net_buf_simple_pull_u8(&buf);

	if (!BT_BAP_BASS_VALID_OPCODE(opcode)) {
		return BT_GATT_ERR(BT_BAP_BASS_ERR_OPCODE_NOT_SUPPORTED);
	}

	LOG_HEXDUMP_DBG(data, len, "Data");

	switch (opcode) {
	case BT_BAP_BASS_OP_SCAN_STOP:
		LOG_DBG("Assistant stopping scanning");

		if (buf.len != 0) {
			LOG_DBG("Invalid length %u", buf.size);
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}

		if (scan_delegator_cbs != NULL && scan_delegator_cbs->scanning_state != NULL) {
			scan_delegator_cbs->scanning_state(conn, false);
		}

		break;
	case BT_BAP_BASS_OP_SCAN_START:
		LOG_DBG("Assistant starting scanning");

		if (buf.len != 0) {
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}

		if (scan_delegator_cbs != NULL && scan_delegator_cbs->scanning_state != NULL) {
			scan_delegator_cbs->scanning_state(conn, true);
		}

		break;
	case BT_BAP_BASS_OP_ADD_SRC:
		LOG_DBG("Assistant adding source");

		err = scan_delegator_add_src(conn, &buf);
		if (err != 0) {
			LOG_DBG("Could not add source %d", err);
			return err;
		}

		break;
	case BT_BAP_BASS_OP_MOD_SRC:
		LOG_DBG("Assistant modifying source");

		err = scan_delegator_mod_src(conn, &buf);
		if (err != 0) {
			LOG_DBG("Could not modify source %d", err);
			return err;
		}

		break;
	case BT_BAP_BASS_OP_BROADCAST_CODE:
		LOG_DBG("Assistant setting broadcast code");

		err = scan_delegator_broadcast_code(conn, &buf);
		if (err != 0) {
			LOG_DBG("Could not set broadcast code");
			return err;
		}

		break;
	case BT_BAP_BASS_OP_REM_SRC:
		LOG_DBG("Assistant removing source");

		err = scan_delegator_rem_src(conn, &buf);
		if (err != 0) {
			LOG_DBG("Could not remove source %d", err);
			return err;
		}

		break;
	default:
		break;
	}

	return len;
}

static void recv_state_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_recv_state(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint8_t idx = POINTER_TO_UINT(BT_AUDIO_CHRC_USER_DATA(attr));
	struct bass_recv_state_internal *recv_state = &scan_delegator.recv_states[idx];
	struct bt_bap_scan_delegator_recv_state *state = &recv_state->state;

	if (recv_state->active) {
		ssize_t ret_val;
		int err;

		LOG_DBG("Index %u: Source ID 0x%02x", idx, state->src_id);

		err = k_mutex_lock(&recv_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
		if (err != 0) {
			LOG_DBG("Failed to lock mutex: %d", err);

			return err;
		}

		if (IS_ENABLED(CONFIG_BT_BAP_SCAN_DELEGATOR_LOG_LEVEL_DBG)) {
			bt_debug_dump_recv_state(recv_state);
		}
		net_buf_put_recv_state(recv_state);

		ret_val = bt_gatt_attr_read(conn, attr, buf, len, offset,
					    read_buf.data, read_buf.len);

		err = k_mutex_unlock(&recv_state->mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

		return ret_val;
	}
	LOG_DBG("Index %u: Not active", idx);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);

}

#define RECEIVE_STATE_CHARACTERISTIC(idx) \
	BT_AUDIO_CHRC(BT_UUID_BASS_RECV_STATE, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,\
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_recv_state, NULL, UINT_TO_POINTER(idx)), \
	BT_AUDIO_CCC(recv_state_cfg_changed)

static struct bt_gatt_attr attr_bass_svc[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BASS),
	BT_AUDIO_CHRC(BT_UUID_BASS_CONTROL_POINT,
		      BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
		      BT_GATT_PERM_WRITE_ENCRYPT,
		      NULL, write_control_point, NULL),
	RECEIVE_STATE_CHARACTERISTIC(0),
#if CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 1
	RECEIVE_STATE_CHARACTERISTIC(1),
#if CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 2
	RECEIVE_STATE_CHARACTERISTIC(2)
#endif /* CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 2 */
#endif /* CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 1 */
};

static struct bt_gatt_service bass_svc = BT_GATT_SERVICE(attr_bass_svc);

static int bass_register(void)
{
	int err;

	err = bt_gatt_service_register(&bass_svc);
	if (err) {
		LOG_DBG("Failed to register BASS service (err %d)", err);
		return err;
	}

	LOG_DBG("BASS service registered");

	return 0;
}

static int bass_unregister(void)
{
	int err;

	err = bt_gatt_service_unregister(&bass_svc);
	if (err) {
		LOG_DBG("Failed to unregister BASS service (err %d)", err);
		return err;
	}

	LOG_DBG("BASS service unregistered");

	return 0;
}

/****************************** PUBLIC API ******************************/
int bt_bap_scan_delegator_register(struct bt_bap_scan_delegator_cb *cb)
{
	int err;

	if (atomic_test_and_set_bit(scan_delegator_flags,
				    SCAN_DELEGATOR_FLAG_REGISTERED_SCAN_DELIGATOR)) {
		LOG_DBG("Scan delegator already registered");
		return -EALREADY;
	}

	err = bass_register();
	if (err) {
		atomic_clear_bit(scan_delegator_flags,
				 SCAN_DELEGATOR_FLAG_REGISTERED_SCAN_DELIGATOR);
		return err;
	}

	/* Store the pointer to the first characteristic in each receive state */
	scan_delegator.recv_states[0].attr = &bass_svc.attrs[3];
	scan_delegator.recv_states[0].index = 0;
#if CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 1
	scan_delegator.recv_states[1].attr = &bass_svc.attrs[6];
	scan_delegator.recv_states[1].index = 1;
#if CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 2
	scan_delegator.recv_states[2].attr = &bass_svc.attrs[9];
	scan_delegator.recv_states[2].index = 2;
#endif /* CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 2 */
#endif /* CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 1 */

	for (size_t i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		struct bass_recv_state_internal *internal_state = &scan_delegator.recv_states[i];

		err = k_mutex_init(&internal_state->mutex);
		__ASSERT(err == 0, "Failed to initialize mutex");

		k_work_init_delayable(&internal_state->notify_work, notify_work_handler);
	}

	scan_delegator_cbs = cb;

	return 0;
}

int bt_bap_scan_delegator_unregister(void)
{
	int err;

	if (!atomic_test_and_clear_bit(scan_delegator_flags,
				       SCAN_DELEGATOR_FLAG_REGISTERED_SCAN_DELIGATOR)) {
		LOG_DBG("Scan delegator not yet registered");
		return -EALREADY;
	}

	err = bass_unregister();
	if (err) {
		atomic_set_bit(scan_delegator_flags,
			       SCAN_DELEGATOR_FLAG_REGISTERED_SCAN_DELIGATOR);
		return err;
	}

	scan_delegator_cbs = NULL;

	return 0;
}

int bt_bap_scan_delegator_set_pa_state(uint8_t src_id,
				       enum bt_bap_pa_state pa_state)
{
	struct bass_recv_state_internal *internal_state = bass_lookup_src_id(src_id);
	struct bt_bap_scan_delegator_recv_state *recv_state;
	bool state_changed = false;
	int err;

	if (internal_state == NULL) {
		LOG_DBG("Could not find recv_state by src_id %u", src_id);
		return -EINVAL;
	}

	err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to lock mutex: %d", err);

		return -EBUSY;
	}

	recv_state = &internal_state->state;

	if (recv_state->pa_sync_state != pa_state) {
		recv_state->pa_sync_state = pa_state;
		set_receive_state_changed(internal_state);
		state_changed = true;
	}

	err = k_mutex_unlock(&internal_state->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	if (state_changed) {
		/* app callback */
		receive_state_updated(NULL, internal_state);
	}

	return 0;
}

int bt_bap_scan_delegator_set_bis_sync_state(
	uint8_t src_id,
	uint32_t bis_synced[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS])
{
	struct bass_recv_state_internal *internal_state = bass_lookup_src_id(src_id);
	bool notify = false;
	int ret = 0;
	int err;

	if (internal_state == NULL) {
		LOG_DBG("Could not find recv_state by src_id %u", src_id);
		return -EINVAL;
	}

	err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to lock mutex: %d", err);

		return -EBUSY;
	}

	if (internal_state->state.pa_sync_state != BT_BAP_PA_STATE_SYNCED) {
		LOG_DBG("PA for src_id %u isn't synced, cannot be BIG synced",
			src_id);
		ret = -EINVAL;
		goto unlock_return;
	}

	/* Verify state for all subgroups before assigning any data */
	for (uint8_t i = 0U; i < internal_state->state.num_subgroups; i++) {
		if (i >= CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
			break;
		}

		if (bis_synced[i] == BT_BAP_BIS_SYNC_NO_PREF ||
		    !bits_subset_of(bis_synced[i],
				    internal_state->requested_bis_sync[i])) {
			LOG_DBG("Subgroup[%u] invalid bis_sync value %x for %x",
				i, bis_synced[i], internal_state->requested_bis_sync[i]);
			ret = -EINVAL;
			goto unlock_return;
		}
	}

	for (uint8_t i = 0U; i < internal_state->state.num_subgroups; i++) {
		struct bt_bap_bass_subgroup *subgroup =
			&internal_state->state.subgroups[i];

		if (i >= CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
			break;
		}

		if (bis_synced[i] != subgroup->bis_sync) {
			notify = true;
			subgroup->bis_sync = bis_synced[i];
		}
	}

	LOG_DBG("Index %u: Source ID 0x%02x synced",
		internal_state->index, src_id);

	if (internal_state->state.encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE) {
		(void)memset(internal_state->state.bad_code, 0xFF,
			     sizeof(internal_state->state.bad_code));
	}

	if (notify) {
		set_receive_state_changed(internal_state);
	}

unlock_return:
	err = k_mutex_unlock(&internal_state->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	if (notify) {
		/* app callback */
		receive_state_updated(NULL, internal_state);
	}

	return ret;
}

static bool valid_bt_bap_scan_delegator_add_src_param(
	const struct bt_bap_scan_delegator_add_src_param *param)
{
	uint32_t aggregated_bis_syncs = 0U;

	if (param->broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		LOG_DBG("Invalid broadcast_id: %u", param->broadcast_id);

		return false;
	}

	CHECKIF(param->addr.type > BT_ADDR_LE_RANDOM) {
		LOG_DBG("param->addr.type %u is invalid", param->addr.type);
		return false;
	}

	CHECKIF(param->sid > BT_GAP_SID_MAX) {
		LOG_DBG("param->sid %d is invalid", param->sid);
		return false;
	}

	if (param->num_subgroups > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		LOG_WRN("Too many subgroups %u/%u",
			param->num_subgroups,
			CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);

		return false;
	}

	if (!IN_RANGE(param->pa_state, BT_BAP_PA_STATE_NOT_SYNCED, BT_BAP_PA_STATE_NO_PAST)) {
		LOG_DBG("Invalid PA state: %d", param->pa_state);

		return false;
	}

	for (uint8_t i = 0U; i < param->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *subgroup = &param->subgroups[i];

		if (!bis_syncs_unique_or_no_pref(subgroup->bis_sync,
						 aggregated_bis_syncs)) {
			LOG_DBG("Invalid BIS sync: %u", subgroup->bis_sync);

			return false;
		}

		if (subgroup->metadata_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
			LOG_DBG("subgroup[%u]: Invalid metadata_len: %u",
				i, subgroup->metadata_len);

			return false;
		}
	}

	return true;
}

int bt_bap_scan_delegator_add_src(const struct bt_bap_scan_delegator_add_src_param *param)
{
	struct bass_recv_state_internal *internal_state = NULL;
	struct bt_bap_scan_delegator_recv_state *state;
	int err;

	CHECKIF(!valid_bt_bap_scan_delegator_add_src_param(param)) {
		return -EINVAL;
	}

	internal_state = bass_lookup_state(param->addr.type, param->sid, param->broadcast_id);
	if (internal_state != NULL) {
		LOG_DBG("Adding addr.type=0x%02X adv_sid=0x%02X and broadcast_id=0x%06X would "
			"result in duplication",
			param->addr.type, param->sid, param->broadcast_id);

		return -EALREADY;
	}

	internal_state = get_free_recv_state();
	if (internal_state == NULL) {
		LOG_DBG("Could not get free receive state");

		return -ENOMEM;
	}

	err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to lock mutex: %d", err);

		return -EBUSY;
	}

	state = &internal_state->state;

	state->src_id = next_src_id();
	bt_addr_le_copy(&state->addr, &param->addr);
	state->adv_sid = param->sid;
	state->broadcast_id = param->broadcast_id;
	state->pa_sync_state = param->pa_state;
	state->num_subgroups = param->num_subgroups;
	if (state->num_subgroups > 0U) {
		(void)memcpy(state->subgroups, param->subgroups,
			     sizeof(state->subgroups));
	} else {
		(void)memset(state->subgroups, 0, sizeof(state->subgroups));
	}

	internal_state->active = true;

	/* Set all requested_bis_sync to BT_BAP_BIS_SYNC_NO_PREF, as no
	 * Broadcast Assistant has set any requests yet
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(internal_state->requested_bis_sync); i++) {
		internal_state->requested_bis_sync[i] = BT_BAP_BIS_SYNC_NO_PREF;
	}

	LOG_DBG("Index %u: New source added: ID 0x%02x",
		internal_state->index, state->src_id);

	set_receive_state_changed(internal_state);

	err = k_mutex_unlock(&internal_state->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	/* app callback */
	receive_state_updated(NULL, internal_state);

	return state->src_id;
}

static bool valid_bt_bap_scan_delegator_mod_src_param(
	const struct bt_bap_scan_delegator_mod_src_param *param)
{
	uint32_t aggregated_bis_syncs = 0U;

	if (param->broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		LOG_DBG("Invalid broadcast_id: %u", param->broadcast_id);

		return false;
	}

	if (param->num_subgroups > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		LOG_WRN("Too many subgroups %u/%u",
			param->num_subgroups,
			CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);

		return false;
	}

	for (uint8_t i = 0U; i < param->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *subgroup = &param->subgroups[i];

		if (subgroup->bis_sync != BT_BAP_BIS_SYNC_FAILED &&
		    !bis_syncs_unique_or_no_pref(subgroup->bis_sync, aggregated_bis_syncs)) {
			LOG_DBG("Invalid BIS sync: %u", subgroup->bis_sync);

			return false;
		}

		if (subgroup->metadata_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
			LOG_DBG("subgroup[%u]: Invalid metadata_len: %u",
				i, subgroup->metadata_len);

			return false;
		}
	}

	return true;
}

int bt_bap_scan_delegator_mod_src(const struct bt_bap_scan_delegator_mod_src_param *param)
{
	struct bass_recv_state_internal *internal_state = NULL;
	struct bt_bap_scan_delegator_recv_state *state;
	bool state_changed = false;
	int ret = 0;
	int err;

	CHECKIF(!valid_bt_bap_scan_delegator_mod_src_param(param)) {
		return -EINVAL;
	}

	internal_state = bass_lookup_src_id(param->src_id);
	if (internal_state == NULL) {
		LOG_DBG("Could not find receive state with src_id %u", param->src_id);

		return -ENOENT;
	}

	err = k_mutex_lock(&internal_state->mutex, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to lock mutex: %d", err);

		return -EBUSY;
	}

	state = &internal_state->state;

	if (state->broadcast_id != param->broadcast_id) {
		state->broadcast_id = param->broadcast_id;
		state_changed = true;
	}

	if (state->num_subgroups != param->num_subgroups) {
		state->num_subgroups = param->num_subgroups;
		state_changed = true;
	}

	if (state->encrypt_state != param->encrypt_state) {
		state->encrypt_state = param->encrypt_state;

		if (state->encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE) {
			(void)memset(state->bad_code, 0xFF, sizeof(internal_state->state.bad_code));
		}

		state_changed = true;
	}

	/* Verify that the BIS sync values is acceptable for the receive state */
	for (uint8_t i = 0U; i < state->num_subgroups; i++) {
		const uint32_t bis_sync = param->subgroups[i].bis_sync;
		const uint32_t bis_sync_requested = internal_state->requested_bis_sync[i];

		if (bis_sync != BT_BAP_BIS_SYNC_FAILED &&
		    !bits_subset_of(bis_sync, bis_sync_requested)) {
			LOG_DBG("Subgroup[%d] invalid bis_sync value %x for %x",
				i, bis_sync, bis_sync_requested);
			ret = -EINVAL;
			goto unlock_return;
		}
	}

	for (uint8_t i = 0U; i < state->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *param_subgroup = &param->subgroups[i];
		struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];

		if (subgroup->bis_sync != param_subgroup->bis_sync) {
			subgroup->bis_sync = param_subgroup->bis_sync;
			state_changed = true;
		}

		/* If the metadata len is 0, we shall not overwrite the existing metadata */
		if (param_subgroup->metadata_len == 0U) {
			continue;
		}

		if (subgroup->metadata_len != param_subgroup->metadata_len) {
			subgroup->metadata_len = param_subgroup->metadata_len;
			state_changed = true;
		}

		if (subgroup->metadata_len != param_subgroup->metadata_len ||
		    memcmp(subgroup->metadata, param_subgroup->metadata,
			   param_subgroup->metadata_len) != 0) {
			(void)memcpy(subgroup->metadata,
				     param_subgroup->metadata,
				     param_subgroup->metadata_len);
			subgroup->metadata_len = param_subgroup->metadata_len;
			state_changed = true;
		}
	}

	if (state_changed) {
		LOG_DBG("Index %u: Source modified: ID 0x%02x",
			internal_state->index, state->src_id);
		set_receive_state_changed(internal_state);
	}

unlock_return:
	err = k_mutex_unlock(&internal_state->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);


	if (state_changed) {
		/* app callback */
		receive_state_updated(NULL, internal_state);
	}

	return ret;
}

int bt_bap_scan_delegator_rem_src(uint8_t src_id)
{
	struct net_buf_simple buf;

	net_buf_simple_init_with_data(&buf, (void *)&src_id, sizeof(src_id));

	if (scan_delegator_rem_src(NULL, &buf) == 0) {
		return 0;
	} else {
		return -EINVAL;
	}
}

void bt_bap_scan_delegator_foreach_state(bt_bap_scan_delegator_state_func_t func, void *user_data)
{
	for (size_t i = 0U; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		if (scan_delegator.recv_states[i].active) {
			bool stop;

			stop = func(&scan_delegator.recv_states[i].state, user_data);
			if (stop) {
				return;
			}
		}
	}
}

struct scan_delegator_state_find_state_param {
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	bt_bap_scan_delegator_state_func_t func;
	void *user_data;
};

static bool
scan_delegator_state_find_state_cb(const struct bt_bap_scan_delegator_recv_state *recv_state,
				   void *user_data)
{
	struct scan_delegator_state_find_state_param *param = user_data;
	bool found;

	found = param->func(recv_state, param->user_data);
	if (found) {
		param->recv_state = recv_state;

		return true;
	}

	return false;
}

const struct bt_bap_scan_delegator_recv_state *
bt_bap_scan_delegator_find_state(bt_bap_scan_delegator_state_func_t func, void *user_data)
{
	struct scan_delegator_state_find_state_param param = {
		.recv_state = NULL,
		.func = func,
		.user_data = user_data,
	};

	bt_bap_scan_delegator_foreach_state(scan_delegator_state_find_state_cb, &param);

	return param.recv_state;
}

const struct bt_bap_scan_delegator_recv_state *bt_bap_scan_delegator_lookup_src_id(uint8_t src_id)
{
	const struct bass_recv_state_internal *internal_state = bass_lookup_src_id(src_id);

	if (internal_state != NULL) {
		return &internal_state->state;
	}

	return NULL;
}
